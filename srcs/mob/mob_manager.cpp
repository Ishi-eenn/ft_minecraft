#include "mob/mob_manager.hpp"
#include "world/world.hpp"
#include "types.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ── Physics constants ─────────────────────────────────────────────────────────
static constexpr float GRAVITY        = 28.0f;
static constexpr float TERMINAL_VEL  = -50.0f;
static constexpr float ZOMBIE_HALF_W = 0.30f;
static constexpr float ZOMBIE_HEIGHT = 1.80f;

// ── AI constants ──────────────────────────────────────────────────────────────
static constexpr float ZOMBIE_SPEED   = 0.9f;   // blocks/sec
static constexpr float DETECT_RANGE  = 16.0f;  // start chasing
static constexpr float LOSE_RANGE    = 26.0f;  // give up chasing
static constexpr float ATTACK_RANGE  = 1.5f;   // deal damage
static constexpr float ATTACK_DAMAGE = 2.0f;   // HP per hit
static constexpr float ATTACK_PERIOD = 1.0f;   // seconds between hits
static constexpr float PLAYER_EYE_H  = 1.62f;

static constexpr float CREEPER_SPEED         = 1.05f;
static constexpr float CREEPER_FUSE_RANGE    = 1.8f;
static constexpr float CREEPER_CANCEL_RANGE  = 3.0f;
static constexpr float CREEPER_FUSE_Y_RANGE  = 1.2f;
static constexpr float CREEPER_FUSE_TIME     = 1.5f;
static constexpr float CREEPER_BLOCK_RADIUS  = 3.2f;
static constexpr float CREEPER_DAMAGE_RADIUS = 5.0f;
static constexpr float CREEPER_MAX_DAMAGE    = 14.0f;

// ── AABB collision ────────────────────────────────────────────────────────────
bool MobManager::overlapsAny(float x, float y, float z,
                               const std::function<bool(int,int,int)>& isSolid) {
    static constexpr float EPS = 1e-4f;
    const int x0 = (int)std::floor(x - ZOMBIE_HALF_W + EPS);
    const int x1 = (int)std::floor(x + ZOMBIE_HALF_W - EPS);
    const int y0 = (int)std::floor(y + EPS);
    const int y1 = (int)std::floor(y + ZOMBIE_HEIGHT - EPS);
    const int z0 = (int)std::floor(z - ZOMBIE_HALF_W + EPS);
    const int z1 = (int)std::floor(z + ZOMBIE_HALF_W - EPS);
    for (int bx = x0; bx <= x1; ++bx)
        for (int by = y0; by <= y1; ++by)
            for (int bz = z0; bz <= z1; ++bz)
                if (isSolid(bx, by, bz)) return true;
    return false;
}

void MobManager::applyMovement(Zombie& z, float dt, float move_x, float move_z,
                               const std::function<bool(int,int,int)>& isSolid) {
    // ── Gravity ───────────────────────────────────────────────────────────
    z.velocity_y -= GRAVITY * dt;
    if (z.velocity_y < TERMINAL_VEL) z.velocity_y = TERMINAL_VEL;

    // ── AABB: X ───────────────────────────────────────────────────────────
    if (move_x != 0 && !overlapsAny(z.x + move_x, z.y, z.z, isSolid))
        z.x += move_x;

    // ── AABB: Z ───────────────────────────────────────────────────────────
    if (move_z != 0 && !overlapsAny(z.x, z.y, z.z + move_z, isSolid))
        z.z += move_z;

    // ── AABB: Y ───────────────────────────────────────────────────────────
    {
        const float ny = z.y + z.velocity_y * dt;
        if (!overlapsAny(z.x, ny, z.z, isSolid)) {
            z.y        = ny;
            z.on_ground = false;
        } else {
            if (z.velocity_y < 0) z.on_ground = true;
            z.velocity_y = 0;
        }
    }
}

// Find the y-coordinate of the highest solid block at (x,z), returns -1 if none.
int MobManager::findGroundY(float x, float z, const World& world) const {
    const int bx = (int)std::floor(x);
    const int bz = (int)std::floor(z);
    for (int y = 200; y >= 0; --y) {
        BlockType bt = world.getWorldBlock(bx, y, bz);
        if (bt != BlockType::Air && bt != BlockType::Water &&
            bt != BlockType::ShortGrass && bt != BlockType::Flower &&
            bt != BlockType::Mushroom)
            return y + 1;
    }
    return -1;
}

// ── Target selection ──────────────────────────────────────────────────────────
// 最寄りのプレイヤー（水平距離）を追跡対象に選ぶ。
// マルチプレイでは全プレイヤーが等しく追われる（ホスト専用ではない）。
int MobManager::nearestTarget(const Zombie& z,
                              const std::vector<MobTarget>& targets) {
    int   best   = -1;
    float best_d = 1e30f;
    for (int i = 0; i < (int)targets.size(); ++i) {
        const float dx = targets[i].x - z.x;
        const float dz = targets[i].z - z.z;
        const float d  = dx * dx + dz * dz;
        if (d < best_d) {
            best_d = d;
            best   = i;
        }
    }
    return best;
}

// ── Per-zombie update ─────────────────────────────────────────────────────────
void MobManager::updateZombie(Zombie& z, float dt,
                              const std::vector<MobTarget>& targets,
                              const std::function<bool(int,int,int)>& isSolid,
                              std::vector<float>& out_damage) {
    const int ti = nearestTarget(z, targets);
    // Player feet position
    const float pfx = (ti >= 0) ? targets[ti].x : z.x;
    const float pfz = (ti >= 0) ? targets[ti].z : z.z;

    const float dx     = pfx - z.x;
    const float dz     = pfz - z.z;
    const float horiz  = (ti >= 0) ? std::sqrt(dx * dx + dz * dz) : 1e30f;

    // ── State transitions ─────────────────────────────────────────────────
    z.state_timer += dt;
    switch (z.state) {
    case Zombie::State::Idle:
        if (horiz < DETECT_RANGE) {
            z.state = Zombie::State::Chase;
            z.state_timer = 0;
        }
        break;
    case Zombie::State::Chase:
        if (horiz > LOSE_RANGE) {
            z.state = Zombie::State::Idle;
        } else if (horiz < ATTACK_RANGE) {
            z.state = Zombie::State::Attack;
            z.state_timer = 0;
        }
        break;
    case Zombie::State::Attack:
        if (horiz > ATTACK_RANGE * 1.6f)
            z.state = Zombie::State::Chase;
        break;
    case Zombie::State::Fuse:
        z.state = Zombie::State::Chase;
        break;
    }

    // ── Horizontal movement ───────────────────────────────────────────────
    float move_x = 0, move_z = 0;
    if (z.state == Zombie::State::Chase || z.state == Zombie::State::Attack) {
        if (horiz > 0.02f) {
            const float inv = 1.0f / horiz;
            move_x = dx * inv * ZOMBIE_SPEED * dt;
            move_z = dz * inv * ZOMBIE_SPEED * dt;
            z.yaw  = std::atan2(dz, dx) * (180.0f / 3.14159f);
        }
    } else {
        // Idle: wander slowly
        z.wander_timer -= dt;
        if (z.wander_timer <= 0) {
            z.wander_timer = 2.0f + ((float)rand() / RAND_MAX) * 2.0f;
            z.wander_yaw  += -60.0f + ((float)rand() / RAND_MAX) * 120.0f;
        }
        const float wr = z.wander_yaw * (3.14159f / 180.0f);
        move_x = std::cos(wr) * ZOMBIE_SPEED * 0.35f * dt;
        move_z = std::sin(wr) * ZOMBIE_SPEED * 0.35f * dt;
        z.yaw  = z.wander_yaw;
    }

    applyMovement(z, dt, move_x, move_z, isSolid);

    // ── Walk animation ────────────────────────────────────────────────────
    const bool moving = (move_x != 0 || move_z != 0) && z.on_ground;
    const float spd   = (z.state == Zombie::State::Idle) ? 0.4f : 1.0f;
    if (moving) z.walk_phase += dt * 7.0f * spd;

    // ── Attack cooldown ───────────────────────────────────────────────────
    if (z.attack_cooldown > 0) z.attack_cooldown -= dt;
    if (z.state == Zombie::State::Attack && z.attack_cooldown <= 0 && ti >= 0) {
        z.attack_cooldown = ATTACK_PERIOD;
        out_damage[ti] += ATTACK_DAMAGE;
    }
}

void MobManager::updateCreeper(Zombie& z, float dt,
                               const std::vector<MobTarget>& targets,
                               const std::function<bool(int,int,int)>& isSolid,
                               std::vector<float>& out_damage) {
    const int ti = nearestTarget(z, targets);
    const float pfx = (ti >= 0) ? targets[ti].x : z.x;
    const float pfy = (ti >= 0) ? targets[ti].y - PLAYER_EYE_H : z.y;
    const float pfz = (ti >= 0) ? targets[ti].z : z.z;

    const float dx     = pfx - z.x;
    const float dy     = pfy - z.y;
    const float dz     = pfz - z.z;
    const float horiz  = (ti >= 0) ? std::sqrt(dx * dx + dz * dz) : 1e30f;
    const float ydiff  = std::fabs(dy);
    const bool fuse_close = horiz < CREEPER_FUSE_RANGE &&
                            ydiff < CREEPER_FUSE_Y_RANGE;

    z.state_timer += dt;
    switch (z.state) {
    case Zombie::State::Idle:
        if (horiz < DETECT_RANGE) {
            z.state = Zombie::State::Chase;
            z.state_timer = 0.0f;
        }
        break;
    case Zombie::State::Chase:
        if (horiz > LOSE_RANGE) {
            z.state = Zombie::State::Idle;
        } else if (fuse_close) {
            z.state = Zombie::State::Fuse;
            z.state_timer = 0.0f;
        }
        break;
    case Zombie::State::Attack:
        z.state = Zombie::State::Fuse;
        z.state_timer = 0.0f;
        break;
    case Zombie::State::Fuse:
        if (horiz > CREEPER_CANCEL_RANGE ||
            ydiff > CREEPER_FUSE_Y_RANGE) {
            z.state = Zombie::State::Chase;
            z.fuse_timer = 0.0f;
        }
        break;
    }

    float move_x = 0.0f;
    float move_z = 0.0f;
    if (z.state == Zombie::State::Chase) {
        if (horiz > 0.02f) {
            const float inv = 1.0f / horiz;
            move_x = dx * inv * CREEPER_SPEED * dt;
            move_z = dz * inv * CREEPER_SPEED * dt;
            z.yaw  = std::atan2(dz, dx) * (180.0f / 3.14159f);
        }
        z.fuse_timer = std::max(0.0f, z.fuse_timer - dt * 2.0f);
    } else if (z.state == Zombie::State::Fuse) {
        if (horiz > 0.02f)
            z.yaw = std::atan2(dz, dx) * (180.0f / 3.14159f);
        z.fuse_timer += dt;
    } else {
        z.wander_timer -= dt;
        if (z.wander_timer <= 0) {
            z.wander_timer = 2.0f + ((float)rand() / RAND_MAX) * 2.0f;
            z.wander_yaw  += -60.0f + ((float)rand() / RAND_MAX) * 120.0f;
        }
        const float wr = z.wander_yaw * (3.14159f / 180.0f);
        move_x = std::cos(wr) * CREEPER_SPEED * 0.25f * dt;
        move_z = std::sin(wr) * CREEPER_SPEED * 0.25f * dt;
        z.yaw  = z.wander_yaw;
        z.fuse_timer = std::max(0.0f, z.fuse_timer - dt * 2.0f);
    }

    applyMovement(z, dt, move_x, move_z, isSolid);

    const bool moving = (move_x != 0 || move_z != 0) && z.on_ground;
    if (moving) z.walk_phase += dt * 7.0f;

    if (z.state == Zombie::State::Fuse && z.fuse_timer >= CREEPER_FUSE_TIME) {
        explosions_.push_back({z.x, z.y + 0.85f, z.z, CREEPER_BLOCK_RADIUS});
        z.health = 0.0f;

        // 爆発は追跡対象に限らず、半径内の全プレイヤーにダメージを与える
        for (int i = 0; i < (int)targets.size(); ++i) {
            const float ex = targets[i].x - z.x;
            const float ey = (targets[i].y - PLAYER_EYE_H) - z.y;
            const float ez = targets[i].z - z.z;
            const float dist = std::sqrt(ex * ex + ey * ey + ez * ez);
            if (dist >= CREEPER_DAMAGE_RADIUS) continue;
            const float t = 1.0f - dist / CREEPER_DAMAGE_RADIUS;
            out_damage[i] += CREEPER_MAX_DAMAGE * t * t;
        }
    }
}

// ── Spawning ──────────────────────────────────────────────────────────────────
void MobManager::trySpawn(float px, float pz,
                           const World& world, float time_of_day) {
    if ((int)zombies_.size() >= MAX_ZOMBIES) return;

    // time_of_day wraps 0=midnight, 0.5=noon.
    const bool is_night = (time_of_day > 0.75f || time_of_day < 0.25f);
    if (!is_night) return;

    const float angle = ((float)rand() / RAND_MAX) * 6.28318f;
    const float dist  = 12.0f + ((float)rand() / RAND_MAX) * 10.0f;
    const float sx = px + std::cos(angle) * dist;
    const float sz = pz + std::sin(angle) * dist;

    const int sy = findGroundY(sx, sz, world);
    if (sy < 0) return;

    Zombie z;
    z.x          = sx;
    z.y          = (float)sy;
    z.z          = sz;
    z.yaw        = angle * (180.0f / 3.14159f);
    z.wander_yaw = z.yaw;
    if (((float)rand() / RAND_MAX) < 0.35f) {
        z.type   = MobType::Creeper;
        z.health = 20.0f;
    }
    zombies_.push_back(z);
}

// ── Public API ────────────────────────────────────────────────────────────────
void MobManager::update(float dt,
                        const std::vector<MobTarget>& targets,
                        float time_of_day,
                        const std::function<bool(int,int,int)>& isSolid,
                        const World& world,
                        std::vector<float>& out_damage) {
    out_damage.assign(targets.size(), 0.0f);
    if (targets.empty()) return;

    spawn_timer_ -= dt;
    if (spawn_timer_ <= 0) {
        spawn_timer_ = SPAWN_INTERVAL;
        // スポーン中心はランダムに選んだプレイヤー（全員の周囲に均等に湧かせる）
        const MobTarget& t = targets[rand() % targets.size()];
        trySpawn(t.x, t.z, world, time_of_day);
    }

    for (auto& z : zombies_) {
        if (!z.alive()) continue;
        if (z.type == MobType::Creeper)
            updateCreeper(z, dt, targets, isSolid, out_damage);
        else
            updateZombie(z, dt, targets, isSolid, out_damage);
    }

    zombies_.erase(
        std::remove_if(zombies_.begin(), zombies_.end(),
                       [](const Zombie& z) { return !z.alive(); }),
        zombies_.end());
}

std::vector<MobExplosion> MobManager::consumeExplosions() {
    std::vector<MobExplosion> out = std::move(explosions_);
    explosions_.clear();
    return out;
}

int MobManager::playerMeleeAttack(float px, float py, float pz,
                                   float front_x, float front_z) {
    static constexpr float MELEE_RANGE  = 4.0f;
    static constexpr float MELEE_DAMAGE = 5.0f;

    const float pfx = px, pfy = py - PLAYER_EYE_H, pfz = pz;
    float  best_dist = MELEE_RANGE;
    int    best_idx  = -1;

    for (int i = 0; i < (int)zombies_.size(); ++i) {
        const Zombie& z = zombies_[i];
        if (!z.alive()) continue;
        const float dx = z.x - pfx, dz = z.z - pfz;
        const float dist = std::sqrt(dx * dx + dz * dz);
        if (dist >= best_dist) continue;
        // Must be roughly in front of the player
        const float dot = dx * front_x + dz * front_z;
        if (dot < 0) continue;
        best_dist = dist;
        best_idx  = i;
    }

    if (best_idx >= 0) {
        zombies_[best_idx].health -= MELEE_DAMAGE;
    }
    return best_idx;
}

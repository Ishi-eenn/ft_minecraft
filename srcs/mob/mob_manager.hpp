#pragma once
#include "mob/zombie.hpp"
#include <vector>
#include <functional>
#include <utility>

class World;

struct MobExplosion {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float radius = 0.0f;
};

// モブの追跡・攻撃対象となるプレイヤー1人分の情報。
// マルチプレイではホストがローカル＋全リモートプレイヤーを詰めて渡す。
// id はサーバー割当の player_id（シングルプレイではローカル=0）。
struct MobTarget {
    uint8_t id = 0;
    float   x = 0.0f, y = 0.0f, z = 0.0f;  // eye position
};

class MobManager {
public:
    // Update all mobs.
    // targets = 追跡対象プレイヤー一覧（先頭をローカルとする規約はない。
    //           各モブは最寄りのターゲットを選んで追跡・攻撃する）。
    // out_damage は targets と同サイズに初期化され、
    // 各ターゲットがこのフレームに受けた HP ダメージが入る。
    void update(float dt,
                const std::vector<MobTarget>& targets,
                float time_of_day,
                const std::function<bool(int,int,int)>& isSolid,
                const World& world,
                std::vector<float>& out_damage);

    // Called when the player attacks (left-click + direction).
    // Deals damage to the closest mob within melee range in front of px/py/pz.
    // Returns the mob index that was hit, or -1.
    int playerMeleeAttack(float px, float py, float pz,
                          float front_x, float front_z);

    const std::vector<Zombie>& zombies() const { return zombies_; }
    std::vector<Zombie>&       zombiesMut() { return zombies_; }
    void setZombies(std::vector<Zombie> z) { zombies_ = std::move(z); }
    std::vector<MobExplosion> consumeExplosions();

private:
    void trySpawn(float px, float pz, const World& world, float time_of_day);
    // 最寄りターゲットの index を返す（targets 空なら -1）
    static int nearestTarget(const Zombie& z,
                             const std::vector<MobTarget>& targets);
    void updateZombie(Zombie& z, float dt,
                      const std::vector<MobTarget>& targets,
                      const std::function<bool(int,int,int)>& isSolid,
                      std::vector<float>& out_damage);
    void updateCreeper(Zombie& z, float dt,
                       const std::vector<MobTarget>& targets,
                       const std::function<bool(int,int,int)>& isSolid,
                       std::vector<float>& out_damage);
    static bool overlapsAny(float x, float y, float z,
                             const std::function<bool(int,int,int)>& isSolid);
    static void applyMovement(Zombie& z, float dt, float move_x, float move_z,
                              const std::function<bool(int,int,int)>& isSolid);
    int findGroundY(float x, float z, const World& world) const;

    std::vector<Zombie> zombies_;
    std::vector<MobExplosion> explosions_;
    float spawn_timer_ = 0.0f;

    static constexpr int   MAX_ZOMBIES     = 24;
    static constexpr float SPAWN_INTERVAL  = 5.0f;
};

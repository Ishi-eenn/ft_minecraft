#include "audio/biome_audio_system.hpp"
#include "audio/audio_manager.hpp"
#include <cstdio>

// Biome names must match TerrainGenerator::biomeName() — all uppercase.
void BiomeAudioSystem::init(AudioManager& mgr) {
    mgr_ = &mgr;

    registerBiome("PLAINS",   {"music/plains_bgm.ogg",   "sounds/ambient/plains.ogg"});
    registerBiome("DESERT",   {"music/desert_bgm.ogg",   "sounds/ambient/desert.ogg"});
    registerBiome("TUNDRA",   {"music/tundra_bgm.ogg",   "sounds/ambient/tundra.ogg"});
    registerBiome("ROCKY",    {"music/rocky_bgm.ogg",    "sounds/ambient/rocky.ogg"});
    registerBiome("SWAMP",    {"music/swamp_bgm.ogg",    "sounds/ambient/swamp.ogg"});
    registerBiome("MOUNTAIN", {"music/mountain_bgm.ogg", "sounds/ambient/mountain.ogg"});
    registerBiome("CANYON",   {"music/canyon_bgm.ogg",   "sounds/ambient/canyon.ogg"});
    registerBiome("SPRING",   {"music/spring_bgm.ogg",   "sounds/ambient/spring.ogg"});
    registerBiome("AUTUMN",   {"music/autumn_bgm.ogg",   "sounds/ambient/autumn.ogg"});
}

void BiomeAudioSystem::registerBiome(const std::string& biome_name,
                                     BiomeTrack track) {
    tracks_[biome_name] = std::move(track);
}

void BiomeAudioSystem::update(float dt, const char* current_biome) {
    if (!mgr_ || !current_biome) return;

    // Debounce: prevent thrashing at biome borders
    if (change_cooldown_ > 0.0f) {
        change_cooldown_ -= dt;
        return;
    }

    std::string name(current_biome);
    if (name == current_biome_) return;

    auto it = tracks_.find(name);
    if (it == tracks_.end()) {
        // Unknown biome; record it to avoid repeated warnings but don't crash
        fprintf(stderr, "[BiomeAudio] No track for biome '%s'\n", name.c_str());
        current_biome_ = name;
        return;
    }

    fprintf(stderr, "[BiomeAudio] %s → %s\n",
            current_biome_.empty() ? "(none)" : current_biome_.c_str(),
            name.c_str());

    const BiomeTrack& track = it->second;
    if (!track.bgm_path.empty())
        mgr_->playBgm(track.bgm_path, fade_duration_);
    if (!track.ambient_path.empty())
        mgr_->playAmbient(track.ambient_path, 2.0f);

    current_biome_  = name;
    change_cooldown_ = kChangeCooldown;
}

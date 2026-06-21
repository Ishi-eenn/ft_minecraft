#pragma once
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// NoiseGen — 完全自作の Perlin ノイズ生成器（外部ライブラリ不使用）
//
// subject の General Instructions:
//   "Using pre-built libraries for terrain or biome generation is strictly
//    forbidden. You must implement everything from scratch."
// に従い、勾配ノイズ・フラクタル合成をすべてこのクラス内で実装する。
// 実装本体は noise.cpp を参照。
// ─────────────────────────────────────────────────────────────────────────────
class NoiseGen {
public:
    NoiseGen() = default;

    void setSeed(uint32_t seed);

    // 2D FBm Perlin — base terrain height [-1, 1]
    float getHeight(float x, float z) const;

    // 2D Ridged — valley / erosion mask（高値 = 深い谷）
    float getValley(float x, float z) const;

    // 3D Perlin — cave carving (diagonal tunnels) [-1, 1]
    float getCave(float x, float y, float z) const;

    // 3D Perlin — horizontal cave carving (Y compressed → flat tunnels) [-1, 1]
    float getCaveHoriz(float x, float y, float z) const;

    // 2D low-frequency Perlin — biome temperature [-1, 1]  (-1=cold, +1=hot)
    float getTemperature(float x, float z) const;

    // 2D low-frequency Perlin — biome humidity [-1, 1]  (-1=dry, +1=wet)
    float getHumidity(float x, float z) const;

    // 2D low-frequency Perlin — biome variation [-1, 1] (splits Rocky→Mountain, Desert→Canyon)
    float getVariation(float x, float z) const;

private:
    // 1系統分のノイズ設定（シード・周波数・フラクタル合成パラメータ）。
    // sample2 / sample3 は状態を変更しない純粋関数なので
    // ワーカースレッドから並列に呼んでも安全。
    struct Perlin {
        uint32_t seed       = 0;
        float    frequency  = 0.01f;
        int      octaves    = 1;      // 1 = フラクタル合成なし（単一オクターブ）
        float    lacunarity = 2.0f;   // オクターブごとの周波数倍率
        float    gain       = 0.5f;   // オクターブごとの振幅倍率
        bool     ridged     = false;  // true = Ridged（稜線型）合成
        float    bounding   = 1.0f;   // FBm 合成結果を [-1,1] に収める正規化係数

        void  configure(uint32_t s, float freq, int oct, bool ridged_mode);
        float sample2(float x, float y) const;
        float sample3(float x, float y, float z) const;
    };

    Perlin height_;
    Perlin valley_;
    Perlin cave_;
    Perlin cave_horiz_;
    Perlin temp_;
    Perlin humid_;
    Perlin variation_;

    // シード依存のサンプリングオフセット（Perlin格子点問題の回避）
    float temp_ox_ = 0.0f, temp_oz_ = 0.0f;
    float humid_ox_ = 0.0f, humid_oz_ = 0.0f;
};

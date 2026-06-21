// ─────────────────────────────────────────────────────────────────────────────
// noise.cpp — Perlin ノイズの完全自作実装
//
// 【パーリンノイズとは？】
//   なめらかでランダムな値を生成する関数。
//   単純な乱数（毎回まったく違う値）と違い、隣り合う座標では似た値が返るため
//   「自然に見える」地形・雲・波紋などに使われる。
//
//   仕組み（2Dの場合）:
//     1. 空間を 1×1 の格子に区切り、各格子点にシード由来の「勾配ベクトル」を置く
//     2. サンプル点を囲む4つの格子点について、
//        (勾配ベクトル) ・ (格子点→サンプル点ベクトル) の内積を取る
//     3. 4つの内積をクインティック曲線 6t⁵-15t⁴+10t³ で滑らかに補間する
//   勾配はハッシュ関数で決まるため、同じシード・同じ座標なら常に同じ値が返る
//   （マルチプレイでクライアント同士の地形を一致させるのに必須の決定性）。
//
// 【フラクタル FBm (Fractional Brownian Motion) とは？】
//   ノイズを複数の「周波数（オクターブ）」で重ね合わせる技法。
//   細かい変化（高周波）と大きなうねり（低周波）を合成することで
//   より自然な地形起伏が得られる。
//   Lacunarity: 各オクターブで周波数を何倍にするか（通常は2.0）
//   Gain: 各オクターブで振幅を何倍にするか（通常は0.5で徐々に小さく）
//
// 【Ridged（稜線型）とは？】
//   各オクターブで 1 - 2|noise| を合成する変種。
//   ノイズが 0 に近い場所が鋭い「峰」(+1) になり、山脈や谷の稜線を作れる。
// ─────────────────────────────────────────────────────────────────────────────
#include "world/noise.hpp"
#include <cmath>

namespace {

// ─── 格子点ハッシュ ──────────────────────────────────────────────────────────
// 整数格子座標とシードから 32bit のランダム値を作る。
// 乗算 → XOR シフトを重ねて雪崩効果（1bit の入力差が全 bit に波及）を確保する。
inline uint32_t hash2(uint32_t seed, int32_t x, int32_t y) {
    uint32_t h = seed;
    h ^= (uint32_t)x * 0x653735ABu;
    h ^= (uint32_t)y * 0x9E3779B1u;
    h *= 0x27D4EB2Fu;
    h ^= h >> 15;
    h *= 0x85EBCA77u;
    h ^= h >> 13;
    return h;
}

inline uint32_t hash3(uint32_t seed, int32_t x, int32_t y, int32_t z) {
    uint32_t h = seed;
    h ^= (uint32_t)x * 0x653735ABu;
    h ^= (uint32_t)y * 0x9E3779B1u;
    h ^= (uint32_t)z * 0xC2B2AE3Du;
    h *= 0x27D4EB2Fu;
    h ^= h >> 15;
    h *= 0x85EBCA77u;
    h ^= h >> 13;
    return h;
}

// ─── 勾配ベクトル ────────────────────────────────────────────────────────────
// 2D: 単位円上の8方向（軸4方向 + 対角4方向）。
// 内積の理論最大値は √2/2 ≈ 0.7071 なので、最後に √2 倍して [-1,1] に広げる。
constexpr float kDiag = 0.70710678f;  // √2/2
constexpr float kGrad2[8][2] = {
    { 1.0f,  0.0f}, {-1.0f,  0.0f}, { 0.0f,  1.0f}, { 0.0f, -1.0f},
    { kDiag,  kDiag}, {-kDiag,  kDiag}, { kDiag, -kDiag}, {-kDiag, -kDiag},
};

// 格子点 (ix,iy) の勾配と、格子点→サンプル点ベクトル (fx,fy) の内積
inline float gradDot2(uint32_t seed, int32_t ix, int32_t iy, float fx, float fy) {
    const float* g = kGrad2[hash2(seed, ix, iy) & 7u];
    return g[0] * fx + g[1] * fy;
}

// 3D: 立方体の12辺方向 (±1,±1,0) 系。Improved Perlin (Ken Perlin 2002) と同じ集合。
constexpr float kGrad3[12][3] = {
    {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1},
};

inline float gradDot3(uint32_t seed, int32_t ix, int32_t iy, int32_t iz,
                      float fx, float fy, float fz) {
    const float* g = kGrad3[hash3(seed, ix, iy, iz) % 12u];
    return g[0] * fx + g[1] * fy + g[2] * fz;
}

// ─── 補間 ────────────────────────────────────────────────────────────────────
// クインティック曲線 6t⁵-15t⁴+10t³。
// 1次・2次導関数が格子境界で 0 になるため、継ぎ目が見えない滑らかさになる。
inline float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
inline float lerp(float a, float b, float t) { return a + t * (b - a); }

// ─── 単一オクターブ Perlin ───────────────────────────────────────────────────
// 戻り値はおおよそ [-1, 1]（整数格子点上では必ず 0 になる点に注意）
float perlin2(uint32_t seed, float x, float y) {
    int32_t x0 = (int32_t)std::floor(x);
    int32_t y0 = (int32_t)std::floor(y);
    float fx = x - (float)x0;
    float fy = y - (float)y0;
    float u = fade(fx);
    float v = fade(fy);

    float n00 = gradDot2(seed, x0,     y0,     fx,        fy);
    float n10 = gradDot2(seed, x0 + 1, y0,     fx - 1.0f, fy);
    float n01 = gradDot2(seed, x0,     y0 + 1, fx,        fy - 1.0f);
    float n11 = gradDot2(seed, x0 + 1, y0 + 1, fx - 1.0f, fy - 1.0f);

    // 1.41421356 = 1/(√2/2): 8方向勾配での理論最大値の逆数（[-1,1] へ正規化）
    return lerp(lerp(n00, n10, u), lerp(n01, n11, u), v) * 1.41421356f;
}

float perlin3(uint32_t seed, float x, float y, float z) {
    int32_t x0 = (int32_t)std::floor(x);
    int32_t y0 = (int32_t)std::floor(y);
    int32_t z0 = (int32_t)std::floor(z);
    float fx = x - (float)x0;
    float fy = y - (float)y0;
    float fz = z - (float)z0;
    float u = fade(fx);
    float v = fade(fy);
    float w = fade(fz);

    float n000 = gradDot3(seed, x0,     y0,     z0,     fx,        fy,        fz);
    float n100 = gradDot3(seed, x0 + 1, y0,     z0,     fx - 1.0f, fy,        fz);
    float n010 = gradDot3(seed, x0,     y0 + 1, z0,     fx,        fy - 1.0f, fz);
    float n110 = gradDot3(seed, x0 + 1, y0 + 1, z0,     fx - 1.0f, fy - 1.0f, fz);
    float n001 = gradDot3(seed, x0,     y0,     z0 + 1, fx,        fy,        fz - 1.0f);
    float n101 = gradDot3(seed, x0 + 1, y0,     z0 + 1, fx - 1.0f, fy,        fz - 1.0f);
    float n011 = gradDot3(seed, x0,     y0 + 1, z0 + 1, fx,        fy - 1.0f, fz - 1.0f);
    float n111 = gradDot3(seed, x0 + 1, y0 + 1, z0 + 1, fx - 1.0f, fy - 1.0f, fz - 1.0f);

    float nx00 = lerp(n000, n100, u);
    float nx10 = lerp(n010, n110, u);
    float nx01 = lerp(n001, n101, u);
    float nx11 = lerp(n011, n111, u);

    // 0.96492141 ≈ 12辺勾配セットでの経験的最大値の逆数（[-1,1] へ正規化）
    return lerp(lerp(nx00, nx10, v), lerp(nx01, nx11, v), w) * 0.96492141f;
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// NoiseGen::Perlin — フラクタル合成
// ─────────────────────────────────────────────────────────────────────────────
void NoiseGen::Perlin::configure(uint32_t s, float freq, int oct, bool ridged_mode) {
    seed      = s;
    frequency = freq;
    octaves   = oct;
    ridged    = ridged_mode;

    // FBm 合成後も振幅が [-1,1] に収まるよう、振幅の総和 Σgainⁱ の逆数を掛ける
    float amp_sum = 0.0f;
    float amp = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        amp_sum += amp;
        amp *= gain;
    }
    bounding = 1.0f / amp_sum;
}

float NoiseGen::Perlin::sample2(float x, float y) const {
    float fx = x * frequency;
    float fy = y * frequency;
    float amp = bounding;
    float sum = 0.0f;
    uint32_t s = seed;
    for (int i = 0; i < octaves; ++i) {
        float n = perlin2(s, fx, fy);
        // Ridged: |n| を反転して 0 付近を鋭い峰 (+1) にする
        sum += (ridged ? (1.0f - 2.0f * std::fabs(n)) : n) * amp;
        fx *= lacunarity;
        fy *= lacunarity;
        amp *= gain;
        ++s;  // オクターブごとにシードをずらし、同じ模様の重なりを防ぐ
    }
    return sum;
}

float NoiseGen::Perlin::sample3(float x, float y, float z) const {
    float fx = x * frequency;
    float fy = y * frequency;
    float fz = z * frequency;
    float amp = bounding;
    float sum = 0.0f;
    uint32_t s = seed;
    for (int i = 0; i < octaves; ++i) {
        float n = perlin3(s, fx, fy, fz);
        sum += (ridged ? (1.0f - 2.0f * std::fabs(n)) : n) * amp;
        fx *= lacunarity;
        fy *= lacunarity;
        fz *= lacunarity;
        amp *= gain;
        ++s;
    }
    return sum;
}

// ─────────────────────────────────────────────────────────────────────────────
// setSeed() — シードに基づいて各ノイズの設定を初期化する
//
// シードが同じなら毎回同じ世界が生成される（再現性）。
// 各ノイズのシードは XOR で異なる値にして、似た形にならないようにする。
// ─────────────────────────────────────────────────────────────────────────────
void NoiseGen::setSeed(uint32_t seed) {
    // ── 高さノイズ ── 地形の基本的な起伏を作る ──────────────────────────────
    // Perlin + FBm で「丘と平地が混在する」自然な地形を生成する。
    // Frequency 0.004: 1000ブロックで地形が大きく変化する（バイオームスケール）
    // Octaves 5: 大きなうねり + 小さな凹凸の5層を重ねる
    height_.configure(seed, 0.004f, 5, /*ridged=*/false);

    // ── 谷ノイズ ── 山脈と谷を切り立てる ─────────────────────────────────
    // Ridged FBm: 谷底が鋭く、山頂が丸い「稜線」状の形状を作る。
    valley_.configure(seed ^ 0xCAFEBABEu, 0.007f, 3, /*ridged=*/true);

    // ── 通常洞窟ノイズ ── 斜め方向に伸びる空洞を作る ─────────────────────
    // Frequency 0.020: 0.025 より低くして洞窟スケールを広げる
    // getCave() 内で Y 依存の XZ オフセットを加え、斜め方向のトンネルにする
    cave_.configure(seed ^ 0xDEADBEEFu, 0.020f, 1, /*ridged=*/false);

    // ── 横長洞窟ノイズ ── Y方向を圧縮しつつ斜め方向に伸ばす ──────────────
    // getCaveHoriz() 内で Y に 2.0 を掛けて Y 幅を絞り、
    // さらに Y 依存の XZ オフセットで斜め方向の平たいトンネルにする
    cave_horiz_.configure(seed ^ 0xBEEFDEADu, 0.020f, 1, /*ridged=*/false);

    // ── 気温ノイズ ── バイオームの大きな区分けを作る ────────────────────────
    // ※ Perlin ノイズは整数格子点で必ず 0 を返すため、原点付近は常に Warm になる。
    //   getTemperature() 内でオフセットを加えてスポーン地点を格子点から外す。
    temp_.configure(seed ^ 0xABCD1234u, 0.0009f, 2, /*ridged=*/false);

    // ── 湿度ノイズ ── 気温と組み合わせてバイオームを決める ──────────────────
    // 気温と少し違う周波数（0.0010）にすることでグリッド状の境界線が出にくくなる
    humid_.configure(seed ^ 0x5678EFABu, 0.0010f, 2, /*ridged=*/false);

    // ── バリエーションノイズ ── 境界を自然にずらす ─────────────────────────
    variation_.configure(seed ^ 0x1234ABCDu, 0.0009f, 2, /*ridged=*/false);

    // ── シード依存サンプリングオフセット ────────────────────────────────────
    // Perlin ノイズは整数格子点（ここでは freq×座標 が整数）で必ず 0 を返す。
    // ワールド原点はその格子点になりがちなので、シードから各軸にオフセットを加える。
    // "+0.5f" で格子中心へずらし、seed 由来の項でシードごとに開始バイオームを変える。
    uint32_t ho = seed ^ 0xFEDCBA98u;
    temp_ox_  = 100.5f + (float)( ho         % 400u);
    temp_oz_  = 100.5f + (float)((ho >> 13u) % 400u);
    humid_ox_ = 200.5f + (float)((ho >>  7u) % 400u);
    humid_oz_ = 200.5f + (float)((ho >> 20u) % 400u);
}

// ─────────────────────────────────────────────────────────────────────────────
// ノイズ値の取得関数
// それぞれ -1.0〜+1.0 の範囲の値を返す。
// ─────────────────────────────────────────────────────────────────────────────

// 地形の高さノイズ（2D: X と Z で変化）
float NoiseGen::getHeight(float x, float z) const {
    return height_.sample2(x, z);
}

// 谷・山脈ノイズ（2D）
float NoiseGen::getValley(float x, float z) const {
    return valley_.sample2(x, z);
}

// 斜め洞窟ノイズ（3D: Y に比例した XZ オフセットで斜め方向のトンネルを作る）
float NoiseGen::getCave(float x, float y, float z) const {
    // y が深くなるほど XZ がずれる → 斜め方向に伸びる洞窟
    // y * 0.5 で Y 方向を若干圧縮し、横への広がりを強調
    return cave_.sample3(x + y * 0.40f, y * 0.50f, z + y * 0.25f);
}

// 斜め横長洞窟ノイズ（3D: Y 圧縮 + 斜めオフセット）
// y * 2.0 で縦幅を絞る（y * 3.5 では天井2ブロック未満になりすぎる）
// スパゲッティ式 n1²+n2²<T と組み合わせることで縦幅 ~6-8 ブロックのトンネルを生成する
float NoiseGen::getCaveHoriz(float x, float y, float z) const {
    return cave_horiz_.sample3(x + y * 0.30f, y * 2.0f, z - y * 0.20f);
}

// バイオーム気温ノイズ（2D）
float NoiseGen::getTemperature(float x, float z) const {
    return temp_.sample2(x + temp_ox_, z + temp_oz_);
}

// バイオーム湿度ノイズ（2D）
float NoiseGen::getHumidity(float x, float z) const {
    return humid_.sample2(x + humid_ox_, z + humid_oz_);
}

// バイオームバリエーションノイズ（2D）: 高値=Mountain, 低値=Canyon
float NoiseGen::getVariation(float x, float z) const {
    return variation_.sample2(x, z);
}

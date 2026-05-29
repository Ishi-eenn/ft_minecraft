#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

uniform mat4  uMVP;
uniform mat4  uModel;          // チャンクのワールド変換行列
uniform mat4  uView;           // カメラ視点への変換行列
uniform mat4  uLightSpaceMat;  // 光源視点の (Proj * View)
uniform vec3  uSunDir;         // normalized sun direction (world space)
uniform float uAmbient;        // ambient light level  [0.03 .. 0.35]
uniform float uSunStrength;    // sun diffuse strength  [0.0  .. 0.65]
uniform float uFogStart;
uniform float uFogEnd;

// 松明光源 (Forward point lights, 最大 16 個)
// 距離フォールオフで暖色光を加算する。
uniform vec3  uTorches[16];
uniform int   uTorchCount;     // 実際に有効なエントリ数 (0..16)
uniform float uTorchRange;     // 完全減衰する距離 (ブロック単位)

out vec2  vUV;
out float vLight;
out float vLightShadow;  // 影の中にいるときの明るさ (ambient のみ)
out vec4  vShadowCoord;  // 光源クリップ空間の座標
out float vFogFactor;
out float vTorchLight;   // 松明光の累積強度 (0.0 .. 1.0+)

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vUV = aUV;

    // Static per-face shading (Minecraft style: gives 3-D feel regardless of sun angle)
    float face_shade;
    if      (aNormal.y >  0.5) face_shade = 1.00;   // top:   full bright
    else if (aNormal.y < -0.5) face_shade = 0.50;   // bottom: darkest
    else if (abs(aNormal.x) > 0.5) face_shade = 0.60; // E/W:  medium-dark
    else                           face_shade = 0.80;  // N/S:  medium

    // Dynamic sun contribution
    float sun_diff  = max(dot(aNormal, uSunDir), 0.0);
    vLight       = (uAmbient + uSunStrength * sun_diff) * face_shade;
    vLightShadow = uAmbient * face_shade;  // 太陽光なし (影の中)

    // 光源空間での頂点位置 (PCF shadow sampling に使用)
    vShadowCoord = uLightSpaceMat * uModel * vec4(aPos, 1.0);

    vec3 world_pos = (uModel * vec4(aPos, 1.0)).xyz;

    // ── 松明光源: 全松明からの寄与を線形フォールオフで加算 ──
    // 各光源につき max(0, 1 - dist/range) を加算。複数松明が重なれば明るくなる。
    float torch_accum = 0.0;
    for (int i = 0; i < 16; ++i) {
        if (i >= uTorchCount) break;
        float d = distance(world_pos, uTorches[i]);
        float a = max(0.0, 1.0 - d / uTorchRange);
        torch_accum += a * a;   // 2乗フォールオフでなめらか
    }
    // 面の向きを少し考慮 (上面は明るく、下面は暗く)。完全な点光源では物足りないので
    // top/side/bottom の face_shade を再利用してメリハリを付ける。
    vTorchLight = clamp(torch_accum, 0.0, 1.5) * face_shade;

    vec3 view_pos = (uView * uModel * vec4(aPos, 1.0)).xyz;
    float view_dist = length(view_pos);
    if (uFogEnd > uFogStart + 1.0)
        vFogFactor = smoothstep(uFogStart, uFogEnd, view_dist);
    else
        vFogFactor = 0.0;
}

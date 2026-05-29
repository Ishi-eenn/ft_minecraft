#version 410 core
in vec2 vUV;
in vec3 vViewPos;
in vec3 vViewNormal;

uniform sampler2D uAtlas;

layout(location = 0) out vec4 gNormal;

void main() {
    // 透過テクセル (松明の炎クロスビルボードの余白等) は GBuffer に書き込まないことで
    // SSAO がそれを四角いオクルーダーとして誤検出するのを防ぐ。
    float a = texture(uAtlas, vUV).a;
    if (a < 0.1) discard;

    // ビュー空間の法線を正規化して格納。深度はFBOが自動書き込み。
    gNormal = vec4(normalize(vViewNormal), 0.0);
}

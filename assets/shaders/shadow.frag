#version 410 core
in vec2 vUV;

uniform sampler2D uAtlas;

void main() {
    // 透過テクセル (松明炎ビルボードの余白等) はシャドウマップに深度を書き込まない。
    // これにより透明部分が四角い影として現れる問題を防ぐ。
    float a = texture(uAtlas, vUV).a;
    if (a < 0.1) discard;
    // 深度値はGPUが自動的にDepth Bufferへ書き込む
}

#!/usr/bin/env bash
# gen_placeholder_assets.sh
#
# 開発用: 無音の WAV プレースホルダーを生成する。
# 実際の音声ファイルが手元にない場合でも ft_minecraft が起動できる。
#
# 使い方:
#   bash scripts/gen_placeholder_assets.sh
#
# 必要なもの: python3 (標準インストール済みであること)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASSETS="$REPO_ROOT/assets"

python3 - <<'PYEOF'
import struct
import os
import sys

def make_wav(path, duration_s: float, sample_rate: int = 44100, channels: int = 1):
    """Generate a minimal silent PCM WAV file."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if os.path.exists(path):
        return  # skip existing files

    num_samples = int(sample_rate * duration_s * channels)
    pcm = bytes(num_samples * 2)  # 16-bit silence

    with open(path, 'wb') as f:
        data_size   = len(pcm)
        byte_rate   = sample_rate * channels * 2
        block_align = channels * 2

        f.write(b'RIFF')
        f.write(struct.pack('<I', 36 + data_size))
        f.write(b'WAVE')
        f.write(b'fmt ')
        f.write(struct.pack('<I',  16))                  # chunk size
        f.write(struct.pack('<H',  1))                   # PCM
        f.write(struct.pack('<H',  channels))
        f.write(struct.pack('<I',  sample_rate))
        f.write(struct.pack('<I',  byte_rate))
        f.write(struct.pack('<H',  block_align))
        f.write(struct.pack('<H',  16))                  # bits per sample
        f.write(b'data')
        f.write(struct.pack('<I',  data_size))
        f.write(pcm)

    kb = (44 + data_size) / 1024
    print(f"  created  {os.path.relpath(path)}  ({kb:.0f} KB)")

assets = os.environ.get('ASSETS', 'assets')

# ── BGM (1秒ループ: AudioManager がループ再生するので長さ不要) ────────────────
bgm_biomes = [
    "plains", "desert", "tundra", "rocky", "swamp",
    "mountain", "canyon", "spring", "autumn",
]
print("\n[BGM]")
for b in bgm_biomes:
    make_wav(f"{assets}/music/{b}_bgm.wav", duration_s=1.0)

# ── Ambient (1秒ループ) ──────────────────────────────────────────────────────
print("\n[Ambient]")
for b in bgm_biomes:
    make_wav(f"{assets}/sounds/ambient/{b}.wav", duration_s=1.0)

# ── SE (0.3秒ワンショット) ────────────────────────────────────────────────────
print("\n[SE]")
se_files = [
    "footstep_grass", "footstep_stone", "footstep_sand",
    "footstep_snow",  "footstep_wood",
    "attack", "hurt", "swim",
    "block_break", "block_place",
    "mob_groan", "mob_hurt", "mob_explode",
]
for s in se_files:
    make_wav(f"{assets}/sounds/{s}.wav", duration_s=0.3)

print("\nDone. Replace *.wav with real OGG/WAV files when available.")
PYEOF

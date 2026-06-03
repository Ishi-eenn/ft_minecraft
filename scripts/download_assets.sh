#!/usr/bin/env bash
# download_assets.sh
#
# 使い方:
#   bash scripts/download_assets.sh

set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# =============================================================================
# ここに URL を書く（ファイルが不要なら "" のままでOK → 無音ファイルで代替）
# =============================================================================

# ── BGM ──────────────────────────────────────────────────────────────────────
URL_MUSIC_PLAINS=""
URL_MUSIC_DESERT=""
URL_MUSIC_TUNDRA=""
URL_MUSIC_ROCKY=""
URL_MUSIC_SWAMP=""
URL_MUSIC_MOUNTAIN=""
URL_MUSIC_CANYON=""
URL_MUSIC_SPRING=""
URL_MUSIC_AUTUMN=""

# ── アンビエント ──────────────────────────────────────────────────────────────
URL_AMBIENT_PLAINS=""
URL_AMBIENT_DESERT=""
URL_AMBIENT_TUNDRA=""
URL_AMBIENT_ROCKY=""
URL_AMBIENT_SWAMP=""
URL_AMBIENT_MOUNTAIN=""
URL_AMBIENT_CANYON=""
URL_AMBIENT_SPRING=""
URL_AMBIENT_AUTUMN=""

# ── SE（効果音） ──────────────────────────────────────────────────────────────
URL_SE_FOOTSTEP_GRASS=""
URL_SE_FOOTSTEP_STONE=""
URL_SE_FOOTSTEP_SAND=""
URL_SE_FOOTSTEP_SNOW=""
URL_SE_FOOTSTEP_WOOD=""
URL_SE_ATTACK=""
URL_SE_HURT=""
URL_SE_SWIM=""
URL_SE_BLOCK_BREAK=""
URL_SE_BLOCK_PLACE=""
URL_SE_MOB_GROAN=""
URL_SE_MOB_HURT=""
URL_SE_MOB_EXPLODE=""

# =============================================================================
# 以下は変更不要
# =============================================================================

mkdir -p \
    "$REPO_ROOT/assets/music" \
    "$REPO_ROOT/assets/sounds/ambient" \
    "$REPO_ROOT/assets/sounds"

fetch() {
    local url="$1" dst="$2"
    [[ -z "$url" ]] && return 0          # URL 未設定はスキップ
    [[ -f "$dst" ]] && return 0          # 既にあればスキップ
    echo "  downloading $(basename "$dst") ..."
    if command -v curl &>/dev/null; then
        curl -fsSL -o "$dst" "$url"
    else
        wget -q -O "$dst" "$url"
    fi
}

# BGM
fetch "$URL_MUSIC_PLAINS"    "$REPO_ROOT/assets/music/plains_bgm.ogg"
fetch "$URL_MUSIC_DESERT"    "$REPO_ROOT/assets/music/desert_bgm.ogg"
fetch "$URL_MUSIC_TUNDRA"    "$REPO_ROOT/assets/music/tundra_bgm.ogg"
fetch "$URL_MUSIC_ROCKY"     "$REPO_ROOT/assets/music/rocky_bgm.ogg"
fetch "$URL_MUSIC_SWAMP"     "$REPO_ROOT/assets/music/swamp_bgm.ogg"
fetch "$URL_MUSIC_MOUNTAIN"  "$REPO_ROOT/assets/music/mountain_bgm.ogg"
fetch "$URL_MUSIC_CANYON"    "$REPO_ROOT/assets/music/canyon_bgm.ogg"
fetch "$URL_MUSIC_SPRING"    "$REPO_ROOT/assets/music/spring_bgm.ogg"
fetch "$URL_MUSIC_AUTUMN"    "$REPO_ROOT/assets/music/autumn_bgm.ogg"

# Ambient
fetch "$URL_AMBIENT_PLAINS"    "$REPO_ROOT/assets/sounds/ambient/plains.ogg"
fetch "$URL_AMBIENT_DESERT"    "$REPO_ROOT/assets/sounds/ambient/desert.ogg"
fetch "$URL_AMBIENT_TUNDRA"    "$REPO_ROOT/assets/sounds/ambient/tundra.ogg"
fetch "$URL_AMBIENT_ROCKY"     "$REPO_ROOT/assets/sounds/ambient/rocky.ogg"
fetch "$URL_AMBIENT_SWAMP"     "$REPO_ROOT/assets/sounds/ambient/swamp.ogg"
fetch "$URL_AMBIENT_MOUNTAIN"  "$REPO_ROOT/assets/sounds/ambient/mountain.ogg"
fetch "$URL_AMBIENT_CANYON"    "$REPO_ROOT/assets/sounds/ambient/canyon.ogg"
fetch "$URL_AMBIENT_SPRING"    "$REPO_ROOT/assets/sounds/ambient/spring.ogg"
fetch "$URL_AMBIENT_AUTUMN"    "$REPO_ROOT/assets/sounds/ambient/autumn.ogg"

# SE
fetch "$URL_SE_FOOTSTEP_GRASS"  "$REPO_ROOT/assets/sounds/footstep_grass.ogg"
fetch "$URL_SE_FOOTSTEP_STONE"  "$REPO_ROOT/assets/sounds/footstep_stone.ogg"
fetch "$URL_SE_FOOTSTEP_SAND"   "$REPO_ROOT/assets/sounds/footstep_sand.ogg"
fetch "$URL_SE_FOOTSTEP_SNOW"   "$REPO_ROOT/assets/sounds/footstep_snow.ogg"
fetch "$URL_SE_FOOTSTEP_WOOD"   "$REPO_ROOT/assets/sounds/footstep_wood.ogg"
fetch "$URL_SE_ATTACK"          "$REPO_ROOT/assets/sounds/attack.ogg"
fetch "$URL_SE_HURT"            "$REPO_ROOT/assets/sounds/hurt.ogg"
fetch "$URL_SE_SWIM"            "$REPO_ROOT/assets/sounds/swim.ogg"
fetch "$URL_SE_BLOCK_BREAK"     "$REPO_ROOT/assets/sounds/block_break.ogg"
fetch "$URL_SE_BLOCK_PLACE"     "$REPO_ROOT/assets/sounds/block_place.ogg"
fetch "$URL_SE_MOB_GROAN"       "$REPO_ROOT/assets/sounds/mob_groan.ogg"
fetch "$URL_SE_MOB_HURT"        "$REPO_ROOT/assets/sounds/mob_hurt.ogg"
fetch "$URL_SE_MOB_EXPLODE"     "$REPO_ROOT/assets/sounds/mob_explode.ogg"

# URL が空のファイルは無音 WAV で補完
echo ""
echo "Generating placeholders for missing files..."
ASSETS="$REPO_ROOT/assets" bash "$REPO_ROOT/scripts/gen_placeholder_assets.sh"

echo ""
echo "Done."

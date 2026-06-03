#!/usr/bin/env bash
# download_assets.sh
#
# 音声アセットをダウンロードする。
# リポジトリ合計が 42 MB を超えるため、音声ファイルは Git 管理外。
#
# 使い方:
#   bash scripts/download_assets.sh          # 本番ファイルをダウンロード
#   bash scripts/download_assets.sh --placeholder  # 無音ファイルを生成（開発用）
#
# 実際のファイルをホストする場合:
#   AUDIO_BASE_URL を変更し、ファイル一覧 (MUSIC / AMBIENT / SE) を更新する。
#   推奨ホスト: GitHub Releases の tar.gz アーカイブ

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASSETS="$REPO_ROOT/assets"

# ── ダウンロード元 URL ────────────────────────────────────────────────────────
# GitHub Releases にアーカイブをアップロードしたら以下を書き換える。
# 例: https://github.com/Ishi-eenn/ft_vox/releases/download/v1.0/audio_assets.tar.gz
AUDIO_ARCHIVE_URL=""

# ── ファイル一覧 ──────────────────────────────────────────────────────────────
MUSIC=(
    plains_bgm desert_bgm tundra_bgm rocky_bgm swamp_bgm
    mountain_bgm canyon_bgm spring_bgm autumn_bgm
)
AMBIENT=(
    plains desert tundra rocky swamp
    mountain canyon spring autumn
)
SE=(
    footstep_grass footstep_stone footstep_sand footstep_snow footstep_wood
    attack hurt swim
    block_break block_place
    mob_groan mob_hurt mob_explode
)

# ──────────────────────────────────────────────────────────────────────────────

placeholder_mode=false
if [[ "${1:-}" == "--placeholder" ]]; then
    placeholder_mode=true
fi

# プレースホルダーモード: 無音 WAV を生成して終了
if $placeholder_mode; then
    echo "[download_assets] --placeholder: generating silent WAV files..."
    ASSETS="$ASSETS" bash "$REPO_ROOT/scripts/gen_placeholder_assets.sh"
    exit 0
fi

# URL が未設定なら placeholder にフォールバック
if [[ -z "$AUDIO_ARCHIVE_URL" ]]; then
    echo "[download_assets] AUDIO_ARCHIVE_URL is not set."
    echo "  → Falling back to placeholder generation."
    echo "  To use real audio files, set AUDIO_ARCHIVE_URL in this script"
    echo "  or run:  bash scripts/download_assets.sh --placeholder"
    echo ""
    ASSETS="$ASSETS" bash "$REPO_ROOT/scripts/gen_placeholder_assets.sh"
    exit 0
fi

# ── アーカイブをダウンロードして展開 ─────────────────────────────────────────
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

echo "[download_assets] Downloading $AUDIO_ARCHIVE_URL ..."

if command -v curl &>/dev/null; then
    curl -L --progress-bar -o "$TMP_DIR/audio_assets.tar.gz" "$AUDIO_ARCHIVE_URL"
elif command -v wget &>/dev/null; then
    wget -q --show-progress -O "$TMP_DIR/audio_assets.tar.gz" "$AUDIO_ARCHIVE_URL"
else
    echo "ERROR: curl or wget is required." >&2
    exit 1
fi

echo "[download_assets] Extracting..."
tar -xzf "$TMP_DIR/audio_assets.tar.gz" -C "$TMP_DIR"

# アーカイブ内の *.ogg / *.wav をそれぞれ正しいディレクトリへコピー
mkdir -p "$ASSETS/music" "$ASSETS/sounds/ambient"

install_if_found() {
    local src="$1" dst_dir="$2"
    if [[ -f "$src" ]]; then
        cp "$src" "$dst_dir/"
        echo "  installed  $(basename "$src")"
    fi
}

for name in "${MUSIC[@]}"; do
    for ext in ogg wav; do
        install_if_found "$TMP_DIR/${name}.${ext}" "$ASSETS/music"
    done
done
for name in "${AMBIENT[@]}"; do
    for ext in ogg wav; do
        install_if_found "$TMP_DIR/ambient/${name}.${ext}" "$ASSETS/sounds/ambient"
    done
done
for name in "${SE[@]}"; do
    for ext in ogg wav; do
        install_if_found "$TMP_DIR/${name}.${ext}" "$ASSETS/sounds"
    done
done

echo ""
echo "[download_assets] Done."
du -sh "$ASSETS/music" "$ASSETS/sounds" 2>/dev/null || true

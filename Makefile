BUILD_DIR := build
CONFIGURE := cmake -S . -B $(BUILD_DIR)
BUILD := cmake --build $(BUILD_DIR)
RUN_ARGS := $(filter-out run, $(MAKECMDGOALS))

.PHONY: all configure run clean fclean re sound help

all: configure sound
	$(BUILD)

configure:
	$(CONFIGURE)

run: all
	./ft_minecraft $(RUN_ARGS)

clean:
	if [ -d $(BUILD_DIR) ]; then rm -rf $(BUILD_DIR); fi

fclean: clean
	rm -f ft_minecraft

re: fclean all

sound:
	@bash ./scripts/download_assets.sh
	@bash ./scripts/gen_placeholder_assets.sh

delete-save-data:
	@rm -rf saves
	@echo "セーブデータを削除しました。"

help:
	@echo "使い方:"
	@echo ""
	@echo "  make             ビルド"
	@echo "  make run         ビルド＆シングルプレイ起動（ランダムシード）"
	@echo "  make run 12345   ビルド＆シングルプレイ起動（シード指定）"
	@echo "  make re          フルリビルド"
	@echo "  make clean       ビルドディレクトリを削除"
	@echo "  make fclean      ビルド成果物をすべて削除"
	@echo ""
	@echo "マルチプレイ:"
	@echo ""
	@echo "  【サーバー起動】（ヘッドレス）"
	@echo "    ./ft_minecraft --server [ポート] [シード]"
	@echo "    例) ./ft_minecraft --server 25565 42"
	@echo "        ポート省略時: 25565 / シード省略時: 42"
	@echo ""
	@echo "  【クライアント接続】"
	@echo "    ./ft_minecraft --connect <IPアドレス> [ポート] [シード]"
	@echo "    例) ./ft_minecraft --connect 192.168.0.10 25565 42"
	@echo "        ポート省略時: 25565"


%:
	@:

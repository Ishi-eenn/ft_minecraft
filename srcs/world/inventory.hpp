// ─────────────────────────────────────────────────────────────────────────────
// inventory.hpp — ホットバー (Inventory) 操作のヘルパー関数
//
// engine.cpp 内のローカル static 関数をここに集約し、クラフトシステム等から
// 再利用できるようにする。
// ─────────────────────────────────────────────────────────────────────────────
#pragma once
#include "types.hpp"

// ブロック/アイテムを n 個インベントリに追加する。
// 既存スタックに積む → 空きスロットを探す。スタックを超えた分は捨てる。
// 設置不可な装飾植物 (ShortGrass / Flower / Mushroom) や Air/Water は追加しない。
void inventoryAdd(Inventory& inv, BlockType type, int n = 1);

// 選択スロットから 1 個消費する。成功なら true。
bool inventoryConsumeSelected(Inventory& inv);

// 指定タイプのアイテム数を全スロット合算して返す（クラフト前検査用）。
int  inventoryCount(const Inventory& inv, BlockType type);

// 指定タイプを n 個消費する。在庫が足りていれば true（足りなければ false で何も触らない）。
bool inventoryConsume(Inventory& inv, BlockType type, int n);

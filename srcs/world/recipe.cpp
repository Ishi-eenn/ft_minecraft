// ─────────────────────────────────────────────────────────────────────────────
// recipe.cpp — クラフトレシピデータと検査/適用ロジック
// ─────────────────────────────────────────────────────────────────────────────
#include "recipe.hpp"
#include "inventory.hpp"

const std::vector<Recipe>& getRecipes() {
    static const std::vector<Recipe> kRecipes = {
        {
            "STICK",
            { {BlockType::Wood,  1} },
            { BlockType::Stick, 4 },
        },
        {
            "TORCH",
            { {BlockType::Stick, 1}, {BlockType::Wood, 1} },
            { BlockType::Torch, 4 },
        },
    };
    return kRecipes;
}

bool canCraft(const Inventory& inv, const Recipe& r) {
    for (const Ingredient& in : r.inputs)
        if (inventoryCount(inv, in.type) < in.count)
            return false;
    return true;
}

bool applyCraft(Inventory& inv, const Recipe& r) {
    if (!canCraft(inv, r)) return false;
    for (const Ingredient& in : r.inputs)
        inventoryConsume(inv, in.type, in.count);
    inventoryAdd(inv, r.output.type, r.output.count);
    return true;
}

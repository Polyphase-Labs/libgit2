#pragma once

#if EDITOR

#include <vector>
#include <set>
#include <cstdint>
#include "Maths.h"
#include "Assets/TileMap.h"

class TileMap2D;
class Node;

enum class TileSculptMode : uint8_t
{
    Pencil = 0,
    Eraser,
    Picker,

    Count
};

// One mutated cell in a stroke. Captured both for forward application and
// for reverse-undo.
struct TilePaintChange
{
    int32_t mCellX = 0;
    int32_t mCellY = 0;
    int32_t mLayer = 0;
    TileCell mOldCell;
    TileCell mNewCell;
};

struct TileSculptOptions
{
    TileSculptMode mMode = TileSculptMode::Pencil;
    int32_t mSelectedTileIndex = 0;
    int32_t mActiveLayer = 0;
};

class TilePaintManager
{
public:

    TilePaintManager();
    ~TilePaintManager();

    void Update();
    void HandleNodeDestroy(Node* node);

    TileSculptOptions mOptions;

    // Hover state — read by the paint panel UI.
    bool mHoverValid = false;
    glm::ivec2 mHoverCell = { 0, 0 };

private:

    void UpdateHover();
    void UpdateStroke();
    void CommitStroke();
    void DrawCursor();
    void ApplyBrush(TileMap2D* node, glm::ivec2 cell);

    static int64_t CellKey(int32_t cellX, int32_t cellY, int32_t layer);

    bool mStrokeActive = false;
    TileMap2D* mPendingTarget = nullptr;
    std::vector<TilePaintChange> mPendingChanges;
    std::set<int64_t> mModifiedSet;
};

#endif

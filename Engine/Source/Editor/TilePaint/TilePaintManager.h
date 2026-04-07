#pragma once

#if EDITOR

#include <vector>
#include <set>
#include <string>
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
    RectFill,
    FloodFill,
    Line,
    NineBox,
    Select,

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

    // Index into TileSet::mNineBoxBrushes; -1 means none chosen.
    int32_t mActiveNineBoxIndex = -1;

    // View toggles
    bool mShowCollisionOverlay = false;
    bool mShowTagOverlay = false;
    std::string mTagOverlayName = "";
};

// Snapshot of a rectangular region of tiles, used by Copy/Cut/Paste/Transform.
struct TileClipboard
{
    int32_t mWidth = 0;
    int32_t mHeight = 0;
    std::vector<TileCell> mCells;  // row-major, mWidth * mHeight entries

    bool IsValid() const { return mWidth > 0 && mHeight > 0 && !mCells.empty(); }
    void Clear() { mWidth = 0; mHeight = 0; mCells.clear(); }
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

    // Preview state for drag-based tools (RectFill, Line, NineBox, Select).
    bool mPreviewActive = false;
    glm::ivec2 mDragStartCell = { 0, 0 };

    // Active marquee selection (separate from a stroke in progress).
    bool mHasSelection = false;
    glm::ivec2 mSelectionMin = { 0, 0 };
    glm::ivec2 mSelectionMax = { 0, 0 };

    // Public API the panel calls for clipboard / transform operations.
    void DoCopy();
    void DoCut();
    void DoPaste(glm::ivec2 destOrigin);  // origin = bottom-left cell
    void DoPasteAtCursor();
    void DoFlipClipboardX();
    void DoFlipClipboardY();
    void DoRotateClipboard90();
    bool HasClipboard() const { return mClipboard.IsValid(); }
    bool HasSelection() const { return mHasSelection; }
    glm::ivec2 GetSelectionMin() const { return mSelectionMin; }
    glm::ivec2 GetSelectionMax() const { return mSelectionMax; }
    void ClearSelection() { mHasSelection = false; }

private:

    void UpdateHover();
    void UpdateStroke();
    void CommitStroke();
    void CancelStroke();
    void DrawCursor();
    void DrawOverlays();
    void DrawSelectionOutline();
    void ApplyPencilOrEraser(TileMap2D* node, glm::ivec2 cell);
    void CommitRectFill(TileMap2D* node, glm::ivec2 a, glm::ivec2 b);
    void CommitLine(TileMap2D* node, glm::ivec2 a, glm::ivec2 b);
    void CommitFloodFill(TileMap2D* node, glm::ivec2 origin);
    void CommitNineBox(TileMap2D* node, glm::ivec2 a, glm::ivec2 b);
    void StageCellChange(int32_t cellX, int32_t cellY, int32_t layer,
                         const TileCell& oldCell, const TileCell& newCell);
    TileCell BuildBrushCell(const TileCell& existing) const;
    int32_t PickNineBoxSlot(int32_t cx, int32_t cy,
                            int32_t minX, int32_t maxX,
                            int32_t minY, int32_t maxY) const;

    static int64_t CellKey(int32_t cellX, int32_t cellY, int32_t layer);

    bool mStrokeActive = false;
    TileMap2D* mPendingTarget = nullptr;
    std::vector<TilePaintChange> mPendingChanges;
    std::set<int64_t> mModifiedSet;

    TileClipboard mClipboard;
};

#endif

#if EDITOR

#include "TilePaint/TilePaintManager.h"
#include "EditorState.h"
#include "ActionManager.h"
#include "Viewport3d.h"
#include "Renderer.h"
#include "World.h"
#include "InputDevices.h"
#include "AssetManager.h"
#include "Log.h"

#include "Nodes/3D/Camera3d.h"
#include "Nodes/3D/TileMap2d.h"
#include "Assets/TileMap.h"
#include "Assets/TileSet.h"
#include "Assets/StaticMesh.h"

#include <algorithm>
#include <cmath>
#include <queue>

TilePaintManager::TilePaintManager()
{
}

TilePaintManager::~TilePaintManager()
{
}

int64_t TilePaintManager::CellKey(int32_t cellX, int32_t cellY, int32_t layer)
{
    // Pack signed cell coords + layer into 64 bits for stroke deduplication.
    return (int64_t(uint32_t(cellX) & 0xFFFFFFu)) |
           (int64_t(uint32_t(cellY) & 0xFFFFFFu) << 24) |
           (int64_t(uint32_t(layer)  & 0xFFFFu)   << 48);
}

void TilePaintManager::Update()
{
    Node* selected = GetEditorState()->GetSelectedNode();
    TileMap2D* tileMapNode = nullptr;

    if (selected != nullptr && selected->GetType() == TileMap2D::GetStaticType())
    {
        tileMapNode = static_cast<TileMap2D*>(selected);
    }

    if (tileMapNode == nullptr)
    {
        mHoverValid = false;
        mPreviewActive = false;
        if (mStrokeActive)
        {
            CancelStroke();
        }
        return;
    }

    UpdateHover();
    DrawCursor();
    UpdateStroke();
}

void TilePaintManager::HandleNodeDestroy(Node* node)
{
    if (mPendingTarget == static_cast<TileMap2D*>(node))
    {
        mPendingChanges.clear();
        mModifiedSet.clear();
        mStrokeActive = false;
        mPreviewActive = false;
        mPendingTarget = nullptr;
    }
}

void TilePaintManager::UpdateHover()
{
    mHoverValid = false;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != TileMap2D::GetStaticType())
        return;

    TileMap2D* tileMapNode = static_cast<TileMap2D*>(selected);

    if (!GetEditorState()->GetViewport3D()->ShouldHandleInput())
        return;

    Camera3D* camera = GetWorld(0)->GetActiveCamera();
    if (camera == nullptr)
        return;

    int32_t mouseX, mouseY;
    INP_GetMousePosition(mouseX, mouseY);

    glm::vec3 rayOrigin = camera->GetWorldPosition();
    glm::vec3 rayTarget = camera->ScreenToWorldPosition(mouseX, mouseY);
    glm::vec3 rayDir = rayTarget - rayOrigin;
    if (glm::length(rayDir) < 1e-6f)
        return;
    rayDir = glm::normalize(rayDir);

    TileMap* tileMap = tileMapNode->GetTileMap();
    int32_t layerIndex = mOptions.mActiveLayer;
    float planeZ = 0.0f;
    if (tileMap != nullptr)
    {
        const TileMapLayer* layer = tileMap->GetLayer(layerIndex);
        if (layer != nullptr)
        {
            planeZ = float(layer->mZOrder) * 0.01f;
        }
    }
    planeZ += tileMapNode->GetWorldPosition().z;

    if (std::fabs(rayDir.z) < 1e-6f)
        return;

    float t = (planeZ - rayOrigin.z) / rayDir.z;
    if (t < 0.0f)
        return;

    glm::vec3 hit = rayOrigin + rayDir * t;
    glm::ivec2 cell = tileMapNode->WorldToCell(glm::vec2(hit.x, hit.y));

    mHoverValid = true;
    mHoverCell = cell;
}

void TilePaintManager::DrawCursor()
{
    if (!mHoverValid)
        return;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != TileMap2D::GetStaticType())
        return;

    TileMap2D* tileMapNode = static_cast<TileMap2D*>(selected);
    TileMap* tileMap = tileMapNode->GetTileMap();
    if (tileMap == nullptr)
        return;

    glm::ivec2 tileSize = tileMap->GetTileSize();
    if (tileSize.x <= 0 || tileSize.y <= 0)
        return;

    glm::vec4 color;
    switch (mOptions.mMode)
    {
        case TileSculptMode::Pencil:    color = glm::vec4(0.2f, 0.9f, 0.2f, 0.5f); break;
        case TileSculptMode::Eraser:    color = glm::vec4(0.9f, 0.2f, 0.2f, 0.5f); break;
        case TileSculptMode::Picker:    color = glm::vec4(0.2f, 0.4f, 0.9f, 0.5f); break;
        case TileSculptMode::RectFill:  color = glm::vec4(0.2f, 0.9f, 0.9f, 0.5f); break;
        case TileSculptMode::FloodFill: color = glm::vec4(0.9f, 0.6f, 0.2f, 0.5f); break;
        case TileSculptMode::Line:      color = glm::vec4(0.9f, 0.4f, 0.9f, 0.5f); break;
        case TileSculptMode::NineBox:   color = glm::vec4(0.5f, 0.9f, 0.4f, 0.5f); break;
        case TileSculptMode::Select:    color = glm::vec4(0.9f, 0.9f, 0.9f, 0.5f); break;
        default:                        color = glm::vec4(1.0f); break;
    }

    auto drawCellCube = [&](glm::ivec2 cell)
    {
        glm::vec2 worldXY = tileMapNode->CellCenterToWorld(cell);
        DebugDraw d;
        d.mMesh = LoadAsset<StaticMesh>("SM_Cube");
        d.mTransform =
            glm::translate(glm::mat4(1.0f), glm::vec3(worldXY.x, worldXY.y, tileMapNode->GetWorldPosition().z)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(float(tileSize.x), float(tileSize.y), 1.0f));
        d.mColor = color;
        d.mLife = 0.0f;
        Renderer::Get()->AddDebugDraw(d);
    };

    // Always draw the hover cell.
    drawCellCube(mHoverCell);

    // For drag-based tools mid-stroke, also outline the rect/line preview.
    if (mPreviewActive && mPendingTarget == tileMapNode)
    {
        if (mOptions.mMode == TileSculptMode::RectFill ||
            mOptions.mMode == TileSculptMode::NineBox ||
            mOptions.mMode == TileSculptMode::Select)
        {
            int32_t minX = std::min(mDragStartCell.x, mHoverCell.x);
            int32_t maxX = std::max(mDragStartCell.x, mHoverCell.x);
            int32_t minY = std::min(mDragStartCell.y, mHoverCell.y);
            int32_t maxY = std::max(mDragStartCell.y, mHoverCell.y);

            // Draw corners only to keep DebugDraw count manageable for huge rects.
            drawCellCube({ minX, minY });
            drawCellCube({ maxX, minY });
            drawCellCube({ minX, maxY });
            drawCellCube({ maxX, maxY });
        }
        else if (mOptions.mMode == TileSculptMode::Line)
        {
            drawCellCube(mDragStartCell);
        }
    }

    // Persistent selection outline (drawn whenever a selection exists, not just during drag).
    if (mHasSelection)
    {
        glm::vec4 selColor(0.9f, 0.9f, 0.2f, 0.4f);
        glm::vec4 saved = color;
        color = selColor;
        drawCellCube({ mSelectionMin.x, mSelectionMin.y });
        drawCellCube({ mSelectionMax.x, mSelectionMin.y });
        drawCellCube({ mSelectionMin.x, mSelectionMax.y });
        drawCellCube({ mSelectionMax.x, mSelectionMax.y });
        color = saved;
    }

    // Phase 3 overlays — collision and tag highlights.
    DrawOverlays();
}

void TilePaintManager::DrawOverlays()
{
    if (!mOptions.mShowCollisionOverlay && !mOptions.mShowTagOverlay)
        return;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != TileMap2D::GetStaticType())
        return;

    TileMap2D* tileMapNode = static_cast<TileMap2D*>(selected);
    TileMap* tileMap = tileMapNode->GetTileMap();
    TileSet* tileSet = (tileMap != nullptr) ? tileMap->GetTileSet() : nullptr;
    if (tileMap == nullptr || tileSet == nullptr)
        return;

    glm::ivec2 tileSize = tileMap->GetTileSize();
    if (tileSize.x <= 0 || tileSize.y <= 0)
        return;

    int32_t layerIndex = mOptions.mActiveLayer;
    const TileMapLayer* layer = tileMap->GetLayer(layerIndex);
    if (layer == nullptr)
        return;

    glm::vec4 collisionColor(0.95f, 0.15f, 0.15f, 0.35f);
    glm::vec4 tagColor(0.2f, 0.95f, 0.95f, 0.35f);
    const std::string& tagToHighlight = mOptions.mTagOverlayName;

    // Cap iteration to avoid lagging in huge maps.
    constexpr int32_t kMaxOverlayCells = 4096;
    int32_t drawn = 0;

    for (const auto& kv : layer->mChunks)
    {
        if (drawn >= kMaxOverlayCells) break;

        int32_t cx, cy;
        TileMap::UnpackChunkKey(kv.first, cx, cy);
        int32_t baseX = cx * kTileChunkSize;
        int32_t baseY = cy * kTileChunkSize;

        for (int32_t ly = 0; ly < kTileChunkSize && drawn < kMaxOverlayCells; ++ly)
        {
            for (int32_t lx = 0; lx < kTileChunkSize && drawn < kMaxOverlayCells; ++lx)
            {
                const TileCell& cell = kv.second.mCells[ly * kTileChunkSize + lx];
                if (cell.mTileIndex < 0) continue;

                bool drawCollision = mOptions.mShowCollisionOverlay && tileSet->IsTileSolid(cell.mTileIndex);
                bool drawTag = mOptions.mShowTagOverlay && !tagToHighlight.empty() &&
                               tileSet->HasTileTag(cell.mTileIndex, tagToHighlight);
                if (!drawCollision && !drawTag)
                    continue;

                glm::ivec2 worldCell = { baseX + lx, baseY + ly };
                glm::vec2 worldXY = tileMapNode->CellCenterToWorld(worldCell);
                DebugDraw d;
                d.mMesh = LoadAsset<StaticMesh>("SM_Cube");
                d.mTransform =
                    glm::translate(glm::mat4(1.0f), glm::vec3(worldXY.x, worldXY.y, tileMapNode->GetWorldPosition().z + 0.005f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(float(tileSize.x), float(tileSize.y), 1.0f));
                d.mColor = drawCollision ? collisionColor : tagColor;
                d.mLife = 0.0f;
                Renderer::Get()->AddDebugDraw(d);
                ++drawn;
            }
        }
    }
}

void TilePaintManager::UpdateStroke()
{
    if (!GetEditorState()->GetViewport3D()->ShouldHandleInput())
        return;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != TileMap2D::GetStaticType())
        return;

    TileMap2D* tileMapNode = static_cast<TileMap2D*>(selected);

    // Mouse down: begin appropriate stroke for the active tool.
    if (IsMouseButtonJustDown(MOUSE_LEFT) && mHoverValid)
    {
        switch (mOptions.mMode)
        {
            case TileSculptMode::Picker:
            {
                TileMap* tileMap = tileMapNode->GetTileMap();
                if (tileMap != nullptr)
                {
                    int32_t picked = tileMap->GetTile(mHoverCell.x, mHoverCell.y, mOptions.mActiveLayer);
                    if (picked >= 0)
                        mOptions.mSelectedTileIndex = picked;
                }
                return;
            }

            case TileSculptMode::FloodFill:
            {
                CommitFloodFill(tileMapNode, mHoverCell);
                return;
            }

            case TileSculptMode::RectFill:
            case TileSculptMode::Line:
            case TileSculptMode::NineBox:
            case TileSculptMode::Select:
            {
                mStrokeActive = true;
                mPreviewActive = true;
                mPendingTarget = tileMapNode;
                mDragStartCell = mHoverCell;
                mPendingChanges.clear();
                mModifiedSet.clear();
                return;
            }

            case TileSculptMode::Pencil:
            case TileSculptMode::Eraser:
            default:
            {
                mStrokeActive = true;
                mPendingTarget = tileMapNode;
                mPendingChanges.clear();
                mModifiedSet.clear();
                ApplyPencilOrEraser(tileMapNode, mHoverCell);
                return;
            }
        }
    }

    // Mouse held: continue freehand strokes for pencil/eraser. Rect/Line just
    // update their preview via DrawCursor and apply at mouse-up.
    if (IsMouseButtonDown(MOUSE_LEFT) && mStrokeActive && mHoverValid && mPendingTarget == tileMapNode)
    {
        if (mOptions.mMode == TileSculptMode::Pencil ||
            mOptions.mMode == TileSculptMode::Eraser)
        {
            ApplyPencilOrEraser(tileMapNode, mHoverCell);
        }
    }

    // Mouse up: commit drag-based tools, then close out the stroke.
    if (IsMouseButtonJustUp(MOUSE_LEFT) && mStrokeActive)
    {
        if (mPreviewActive && mPendingTarget != nullptr && mHoverValid)
        {
            switch (mOptions.mMode)
            {
                case TileSculptMode::RectFill:
                    CommitRectFill(mPendingTarget, mDragStartCell, mHoverCell);
                    break;
                case TileSculptMode::Line:
                    CommitLine(mPendingTarget, mDragStartCell, mHoverCell);
                    break;
                case TileSculptMode::NineBox:
                    CommitNineBox(mPendingTarget, mDragStartCell, mHoverCell);
                    break;
                case TileSculptMode::Select:
                {
                    // Selection only updates the marquee state — no cell mutation.
                    int32_t minX = std::min(mDragStartCell.x, mHoverCell.x);
                    int32_t maxX = std::max(mDragStartCell.x, mHoverCell.x);
                    int32_t minY = std::min(mDragStartCell.y, mHoverCell.y);
                    int32_t maxY = std::max(mDragStartCell.y, mHoverCell.y);
                    mSelectionMin = { minX, minY };
                    mSelectionMax = { maxX, maxY };
                    mHasSelection = true;
                    break;
                }
                default:
                    break;
            }
        }

        CommitStroke();
    }
}

TileCell TilePaintManager::BuildBrushCell(const TileCell& existing) const
{
    TileCell out = existing;
    if (mOptions.mMode == TileSculptMode::Eraser)
    {
        out.mTileIndex = -1;
        out.mFlags = 0;
        out.mVariant = 0;
    }
    else
    {
        // Pencil / RectFill / FloodFill / Line all paint the selected index.
        out.mTileIndex = mOptions.mSelectedTileIndex;
    }
    return out;
}

void TilePaintManager::StageCellChange(int32_t cellX, int32_t cellY, int32_t layer,
                                       const TileCell& oldCell, const TileCell& newCell)
{
    if (oldCell.mTileIndex == newCell.mTileIndex &&
        oldCell.mFlags == newCell.mFlags &&
        oldCell.mVariant == newCell.mVariant)
    {
        return;
    }

    int64_t key = CellKey(cellX, cellY, layer);
    if (mModifiedSet.count(key) > 0)
        return;

    TileMap* tileMap = mPendingTarget->GetTileMap();
    tileMap->SetCell(cellX, cellY, newCell, layer);
    mModifiedSet.insert(key);

    TilePaintChange change;
    change.mCellX = cellX;
    change.mCellY = cellY;
    change.mLayer = layer;
    change.mOldCell = oldCell;
    change.mNewCell = newCell;
    mPendingChanges.push_back(change);
}

void TilePaintManager::ApplyPencilOrEraser(TileMap2D* node, glm::ivec2 cell)
{
    TileMap* tileMap = node->GetTileMap();
    if (tileMap == nullptr) return;

    int32_t layer = mOptions.mActiveLayer;
    TileCell oldCell = tileMap->GetCell(cell.x, cell.y, layer);
    TileCell newCell = BuildBrushCell(oldCell);
    StageCellChange(cell.x, cell.y, layer, oldCell, newCell);
}

void TilePaintManager::CommitRectFill(TileMap2D* node, glm::ivec2 a, glm::ivec2 b)
{
    TileMap* tileMap = node->GetTileMap();
    if (tileMap == nullptr) return;

    int32_t layer = mOptions.mActiveLayer;
    int32_t minX = std::min(a.x, b.x);
    int32_t maxX = std::max(a.x, b.x);
    int32_t minY = std::min(a.y, b.y);
    int32_t maxY = std::max(a.y, b.y);

    // Cap the rect at a sane size to avoid runaway brushes.
    constexpr int32_t kMaxRectCells = 1024 * 1024;
    int64_t totalCells = int64_t(maxX - minX + 1) * int64_t(maxY - minY + 1);
    if (totalCells > kMaxRectCells)
    {
        LogWarning("RectFill capped at %d cells (requested %lld)", kMaxRectCells, (long long)totalCells);
        return;
    }

    for (int32_t cy = minY; cy <= maxY; ++cy)
    {
        for (int32_t cx = minX; cx <= maxX; ++cx)
        {
            TileCell oldCell = tileMap->GetCell(cx, cy, layer);
            TileCell newCell = BuildBrushCell(oldCell);
            StageCellChange(cx, cy, layer, oldCell, newCell);
        }
    }
}

void TilePaintManager::CommitLine(TileMap2D* node, glm::ivec2 a, glm::ivec2 b)
{
    TileMap* tileMap = node->GetTileMap();
    if (tileMap == nullptr) return;

    int32_t layer = mOptions.mActiveLayer;

    // Bresenham line between cell coords (handles negative cells natively).
    int32_t x0 = a.x, y0 = a.y;
    int32_t x1 = b.x, y1 = b.y;
    int32_t dx = std::abs(x1 - x0);
    int32_t dy = -std::abs(y1 - y0);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    int32_t safetyCounter = 0;
    constexpr int32_t kMaxLineCells = 16 * 1024;

    while (true)
    {
        TileCell oldCell = tileMap->GetCell(x0, y0, layer);
        TileCell newCell = BuildBrushCell(oldCell);
        StageCellChange(x0, y0, layer, oldCell, newCell);

        if (x0 == x1 && y0 == y1) break;
        if (++safetyCounter > kMaxLineCells)
        {
            LogWarning("Line tool aborted at %d cells", kMaxLineCells);
            break;
        }
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void TilePaintManager::CommitFloodFill(TileMap2D* node, glm::ivec2 origin)
{
    TileMap* tileMap = node->GetTileMap();
    if (tileMap == nullptr) return;

    int32_t layer = mOptions.mActiveLayer;
    int32_t targetTile = tileMap->GetTile(origin.x, origin.y, layer);
    int32_t newTile = mOptions.mSelectedTileIndex;

    // Eraser flood replaces with empty (-1) instead.
    if (mOptions.mMode == TileSculptMode::Eraser)
        newTile = -1;

    if (targetTile == newTile)
        return;

    constexpr int32_t kMaxFillCells = 16384;

    // Stage everything as a single action so undo restores it in one step.
    mStrokeActive = true;
    mPendingTarget = node;
    mPendingChanges.clear();
    mModifiedSet.clear();

    std::queue<glm::ivec2> q;
    q.push(origin);

    int32_t painted = 0;
    while (!q.empty() && painted < kMaxFillCells)
    {
        glm::ivec2 c = q.front();
        q.pop();

        int64_t key = CellKey(c.x, c.y, layer);
        if (mModifiedSet.count(key) > 0)
            continue;

        TileCell oldCell = tileMap->GetCell(c.x, c.y, layer);
        if (oldCell.mTileIndex != targetTile)
            continue;

        TileCell newCell = oldCell;
        newCell.mTileIndex = newTile;
        if (newTile < 0)
        {
            newCell.mFlags = 0;
            newCell.mVariant = 0;
        }

        StageCellChange(c.x, c.y, layer, oldCell, newCell);
        ++painted;

        q.push({ c.x + 1, c.y });
        q.push({ c.x - 1, c.y });
        q.push({ c.x, c.y + 1 });
        q.push({ c.x, c.y - 1 });
    }

    if (painted >= kMaxFillCells)
    {
        LogWarning("FloodFill capped at %d cells", kMaxFillCells);
    }

    CommitStroke();
}

void TilePaintManager::CommitStroke()
{
    if (!mPendingChanges.empty() && mPendingTarget != nullptr)
    {
        ActionManager::Get()->EXE_PaintTiles(mPendingTarget, mPendingChanges);
    }

    mPendingChanges.clear();
    mModifiedSet.clear();
    mStrokeActive = false;
    mPreviewActive = false;
    mPendingTarget = nullptr;
}

void TilePaintManager::CancelStroke()
{
    mPendingChanges.clear();
    mModifiedSet.clear();
    mStrokeActive = false;
    mPreviewActive = false;
    mPendingTarget = nullptr;
}

// ---------------------------------------------------------------------------
// 9-Box brush
// ---------------------------------------------------------------------------

int32_t TilePaintManager::PickNineBoxSlot(int32_t cx, int32_t cy,
                                          int32_t minX, int32_t maxX,
                                          int32_t minY, int32_t maxY) const
{
    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != TileMap2D::GetStaticType())
        return -1;
    TileMap2D* node = static_cast<TileMap2D*>(selected);
    TileMap* tileMap = node->GetTileMap();
    TileSet* tileSet = (tileMap != nullptr) ? tileMap->GetTileSet() : nullptr;
    if (tileSet == nullptr) return -1;

    int32_t brushIdx = mOptions.mActiveNineBoxIndex;
    const auto& brushes = tileSet->GetNineBoxBrushes();
    if (brushIdx < 0 || brushIdx >= int32_t(brushes.size())) return -1;
    const NineBoxBrushDef& brush = brushes[brushIdx];

    int32_t width = maxX - minX + 1;
    int32_t height = maxY - minY + 1;

    // Special cases per spec § "Special cases".
    if (width == 1 && height == 1)
        return brush.mCenter;

    if (width == 1)
    {
        if (cy == maxY) return brush.mTop;
        if (cy == minY) return brush.mBottom;
        return brush.mCenter;
    }

    if (height == 1)
    {
        if (cx == minX) return brush.mLeft;
        if (cx == maxX) return brush.mRight;
        return brush.mCenter;
    }

    bool isLeft  = (cx == minX);
    bool isRight = (cx == maxX);
    bool isTop   = (cy == maxY);  // +Y is up in TileMap2D world space
    bool isBot   = (cy == minY);

    if (isTop && isLeft)  return brush.mTopLeft;
    if (isTop && isRight) return brush.mTopRight;
    if (isBot && isLeft)  return brush.mBottomLeft;
    if (isBot && isRight) return brush.mBottomRight;
    if (isTop)  return brush.mTop;
    if (isBot)  return brush.mBottom;
    if (isLeft) return brush.mLeft;
    if (isRight) return brush.mRight;
    return brush.mCenter;
}

void TilePaintManager::CommitNineBox(TileMap2D* node, glm::ivec2 a, glm::ivec2 b)
{
    TileMap* tileMap = node->GetTileMap();
    if (tileMap == nullptr) return;

    TileSet* tileSet = tileMap->GetTileSet();
    if (tileSet == nullptr) return;

    int32_t brushIdx = mOptions.mActiveNineBoxIndex;
    const auto& brushes = tileSet->GetNineBoxBrushes();
    if (brushIdx < 0 || brushIdx >= int32_t(brushes.size()))
    {
        LogWarning("9-box brush has no active brush selected");
        return;
    }

    int32_t layer = mOptions.mActiveLayer;
    int32_t minX = std::min(a.x, b.x);
    int32_t maxX = std::max(a.x, b.x);
    int32_t minY = std::min(a.y, b.y);
    int32_t maxY = std::max(a.y, b.y);

    constexpr int32_t kMaxRectCells = 1024 * 1024;
    int64_t totalCells = int64_t(maxX - minX + 1) * int64_t(maxY - minY + 1);
    if (totalCells > kMaxRectCells)
    {
        LogWarning("NineBox capped at %d cells", kMaxRectCells);
        return;
    }

    for (int32_t cy = minY; cy <= maxY; ++cy)
    {
        for (int32_t cx = minX; cx <= maxX; ++cx)
        {
            int32_t slotTile = PickNineBoxSlot(cx, cy, minX, maxX, minY, maxY);
            if (slotTile < 0) continue;  // empty slot — leave existing cell

            TileCell oldCell = tileMap->GetCell(cx, cy, layer);
            TileCell newCell = oldCell;
            newCell.mTileIndex = slotTile;
            StageCellChange(cx, cy, layer, oldCell, newCell);
        }
    }
}

// ---------------------------------------------------------------------------
// Selection / clipboard / transforms
// ---------------------------------------------------------------------------

void TilePaintManager::DoCopy()
{
    if (!mHasSelection) return;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != TileMap2D::GetStaticType())
        return;
    TileMap2D* node = static_cast<TileMap2D*>(selected);
    TileMap* tileMap = node->GetTileMap();
    if (tileMap == nullptr) return;

    int32_t layer = mOptions.mActiveLayer;
    int32_t w = mSelectionMax.x - mSelectionMin.x + 1;
    int32_t h = mSelectionMax.y - mSelectionMin.y + 1;
    if (w <= 0 || h <= 0) return;

    mClipboard.mWidth = w;
    mClipboard.mHeight = h;
    mClipboard.mCells.assign(w * h, TileCell{});

    for (int32_t row = 0; row < h; ++row)
    {
        for (int32_t col = 0; col < w; ++col)
        {
            int32_t cx = mSelectionMin.x + col;
            int32_t cy = mSelectionMin.y + row;
            mClipboard.mCells[row * w + col] = tileMap->GetCell(cx, cy, layer);
        }
    }
}

void TilePaintManager::DoCut()
{
    if (!mHasSelection) return;
    DoCopy();

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != TileMap2D::GetStaticType())
        return;
    TileMap2D* node = static_cast<TileMap2D*>(selected);
    TileMap* tileMap = node->GetTileMap();
    if (tileMap == nullptr) return;

    int32_t layer = mOptions.mActiveLayer;
    mStrokeActive = true;
    mPendingTarget = node;
    mPendingChanges.clear();
    mModifiedSet.clear();

    for (int32_t cy = mSelectionMin.y; cy <= mSelectionMax.y; ++cy)
    {
        for (int32_t cx = mSelectionMin.x; cx <= mSelectionMax.x; ++cx)
        {
            TileCell oldCell = tileMap->GetCell(cx, cy, layer);
            if (oldCell.mTileIndex < 0) continue;
            TileCell newCell;  // empty
            StageCellChange(cx, cy, layer, oldCell, newCell);
        }
    }

    CommitStroke();
}

void TilePaintManager::DoPaste(glm::ivec2 destOrigin)
{
    if (!mClipboard.IsValid()) return;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != TileMap2D::GetStaticType())
        return;
    TileMap2D* node = static_cast<TileMap2D*>(selected);
    TileMap* tileMap = node->GetTileMap();
    if (tileMap == nullptr) return;

    int32_t layer = mOptions.mActiveLayer;
    mStrokeActive = true;
    mPendingTarget = node;
    mPendingChanges.clear();
    mModifiedSet.clear();

    for (int32_t row = 0; row < mClipboard.mHeight; ++row)
    {
        for (int32_t col = 0; col < mClipboard.mWidth; ++col)
        {
            const TileCell& src = mClipboard.mCells[row * mClipboard.mWidth + col];
            // Skip empty source cells so paste doesn't erase under-painted tiles.
            if (src.mTileIndex < 0) continue;

            int32_t cx = destOrigin.x + col;
            int32_t cy = destOrigin.y + row;
            TileCell oldCell = tileMap->GetCell(cx, cy, layer);
            StageCellChange(cx, cy, layer, oldCell, src);
        }
    }

    CommitStroke();
}

void TilePaintManager::DoPasteAtCursor()
{
    if (!mHoverValid) return;
    DoPaste(mHoverCell);
}

void TilePaintManager::DoFlipClipboardX()
{
    if (!mClipboard.IsValid()) return;
    int32_t w = mClipboard.mWidth;
    int32_t h = mClipboard.mHeight;
    for (int32_t row = 0; row < h; ++row)
    {
        for (int32_t col = 0; col < w / 2; ++col)
        {
            std::swap(mClipboard.mCells[row * w + col],
                      mClipboard.mCells[row * w + (w - 1 - col)]);
        }
    }
    // Toggle horizontal flip flag on each cell.
    for (TileCell& c : mClipboard.mCells)
        c.mFlags ^= 0x01;
}

void TilePaintManager::DoFlipClipboardY()
{
    if (!mClipboard.IsValid()) return;
    int32_t w = mClipboard.mWidth;
    int32_t h = mClipboard.mHeight;
    for (int32_t row = 0; row < h / 2; ++row)
    {
        for (int32_t col = 0; col < w; ++col)
        {
            std::swap(mClipboard.mCells[row * w + col],
                      mClipboard.mCells[(h - 1 - row) * w + col]);
        }
    }
    for (TileCell& c : mClipboard.mCells)
        c.mFlags ^= 0x02;
}

void TilePaintManager::DoRotateClipboard90()
{
    if (!mClipboard.IsValid()) return;
    int32_t w = mClipboard.mWidth;
    int32_t h = mClipboard.mHeight;
    std::vector<TileCell> rotated(w * h);

    // 90° clockwise: new[col][h-1-row] = old[row][col]
    for (int32_t row = 0; row < h; ++row)
    {
        for (int32_t col = 0; col < w; ++col)
        {
            int32_t newRow = col;
            int32_t newCol = h - 1 - row;
            // After rotation the dimensions swap (h x w).
            rotated[newRow * h + newCol] = mClipboard.mCells[row * w + col];
            rotated[newRow * h + newCol].mFlags ^= 0x04;  // toggle rot90 flag
        }
    }

    mClipboard.mCells = std::move(rotated);
    std::swap(mClipboard.mWidth, mClipboard.mHeight);
}

#endif

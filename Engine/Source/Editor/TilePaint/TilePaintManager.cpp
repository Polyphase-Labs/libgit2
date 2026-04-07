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

TilePaintManager::TilePaintManager()
{
}

TilePaintManager::~TilePaintManager()
{
}

int64_t TilePaintManager::CellKey(int32_t cellX, int32_t cellY, int32_t layer)
{
    // Pack cellX (signed 24-bit), cellY (signed 24-bit), layer (16-bit) into 64 bits
    // for stroke deduplication. Cells outside +/-8 million won't dedupe correctly,
    // but that's fine for any sane map.
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
        if (mStrokeActive)
        {
            CommitStroke();
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

    // Intersect with the active layer's z-plane. The TileMap2D stores tiles
    // at z = layer.zOrder * layerZSpacing relative to the node's local origin.
    TileMap* tileMap = tileMapNode->GetTileMap();
    int32_t layerIndex = mOptions.mActiveLayer;
    float planeZ = 0.0f;
    if (tileMap != nullptr)
    {
        const TileMapLayer* layer = tileMap->GetLayer(layerIndex);
        if (layer != nullptr)
        {
            planeZ = float(layer->mZOrder) * 0.01f;  // matches mLayerZSpacing default
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

    glm::vec2 worldXY = tileMapNode->CellCenterToWorld(mHoverCell);

    glm::vec4 color;
    switch (mOptions.mMode)
    {
        case TileSculptMode::Pencil: color = glm::vec4(0.2f, 0.9f, 0.2f, 0.5f); break;
        case TileSculptMode::Eraser: color = glm::vec4(0.9f, 0.2f, 0.2f, 0.5f); break;
        case TileSculptMode::Picker: color = glm::vec4(0.2f, 0.4f, 0.9f, 0.5f); break;
        default: color = glm::vec4(1.0f); break;
    }

    DebugDraw cursorDraw;
    cursorDraw.mMesh = LoadAsset<StaticMesh>("SM_Cube");
    cursorDraw.mTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(worldXY.x, worldXY.y, tileMapNode->GetWorldPosition().z)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(float(tileSize.x), float(tileSize.y), 1.0f));
    cursorDraw.mColor = color;
    cursorDraw.mLife = 0.0f;
    Renderer::Get()->AddDebugDraw(cursorDraw);
}

void TilePaintManager::UpdateStroke()
{
    if (!GetEditorState()->GetViewport3D()->ShouldHandleInput())
        return;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != TileMap2D::GetStaticType())
        return;

    TileMap2D* tileMapNode = static_cast<TileMap2D*>(selected);

    if (IsMouseButtonJustDown(MOUSE_LEFT) && mHoverValid)
    {
        // Picker is single-click: pick up tile and exit without starting a stroke.
        if (mOptions.mMode == TileSculptMode::Picker)
        {
            TileMap* tileMap = tileMapNode->GetTileMap();
            if (tileMap != nullptr)
            {
                int32_t picked = tileMap->GetTile(mHoverCell.x, mHoverCell.y, mOptions.mActiveLayer);
                if (picked >= 0)
                {
                    mOptions.mSelectedTileIndex = picked;
                }
            }
            return;
        }

        mStrokeActive = true;
        mPendingTarget = tileMapNode;
        mPendingChanges.clear();
        mModifiedSet.clear();

        ApplyBrush(tileMapNode, mHoverCell);
    }
    else if (IsMouseButtonDown(MOUSE_LEFT) && mStrokeActive && mHoverValid && mPendingTarget == tileMapNode)
    {
        ApplyBrush(tileMapNode, mHoverCell);
    }

    if (IsMouseButtonJustUp(MOUSE_LEFT) && mStrokeActive)
    {
        CommitStroke();
    }
}

void TilePaintManager::ApplyBrush(TileMap2D* node, glm::ivec2 cell)
{
    TileMap* tileMap = node->GetTileMap();
    if (tileMap == nullptr)
        return;

    int32_t layer = mOptions.mActiveLayer;
    int64_t key = CellKey(cell.x, cell.y, layer);
    if (mModifiedSet.count(key) > 0)
        return;

    TileCell oldCell = tileMap->GetCell(cell.x, cell.y, layer);
    TileCell newCell = oldCell;

    switch (mOptions.mMode)
    {
        case TileSculptMode::Pencil:
            newCell.mTileIndex = mOptions.mSelectedTileIndex;
            break;
        case TileSculptMode::Eraser:
            newCell.mTileIndex = -1;
            newCell.mFlags = 0;
            newCell.mVariant = 0;
            break;
        case TileSculptMode::Picker:
            // Handled at click time, not during a drag.
            return;
        default:
            return;
    }

    if (newCell.mTileIndex == oldCell.mTileIndex &&
        newCell.mFlags == oldCell.mFlags &&
        newCell.mVariant == oldCell.mVariant)
    {
        return;
    }

    tileMap->SetCell(cell.x, cell.y, newCell, layer);
    mModifiedSet.insert(key);

    TilePaintChange change;
    change.mCellX = cell.x;
    change.mCellY = cell.y;
    change.mLayer = layer;
    change.mOldCell = oldCell;
    change.mNewCell = newCell;
    mPendingChanges.push_back(change);
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
    mPendingTarget = nullptr;
}

#endif

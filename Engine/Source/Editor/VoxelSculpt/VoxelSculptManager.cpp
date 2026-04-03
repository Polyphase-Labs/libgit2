#if EDITOR

#include "VoxelSculpt/VoxelSculptManager.h"
#include "EditorState.h"
#include "ActionManager.h"
#include "Viewport3d.h"
#include "Renderer.h"
#include "World.h"
#include "InputDevices.h"
#include "AssetManager.h"
#include "Log.h"

#include "Nodes/3D/Camera3d.h"
#include "Nodes/3D/Voxel3d.h"
#include "Assets/StaticMesh.h"

VoxelSculptManager::VoxelSculptManager()
{
}

VoxelSculptManager::~VoxelSculptManager()
{
}

uint64_t VoxelSculptManager::VoxelKey(int32_t x, int32_t y, int32_t z)
{
    return (uint64_t(uint32_t(x)) << 40) | (uint64_t(uint32_t(y)) << 20) | uint64_t(uint32_t(z));
}

void VoxelSculptManager::Update()
{
    Node* selected = GetEditorState()->GetSelectedNode();
    Voxel3D* voxel = nullptr;

    if (selected != nullptr && selected->GetType() == Voxel3D::GetStaticType())
    {
        voxel = static_cast<Voxel3D*>(selected);
    }

    if (voxel == nullptr)
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

void VoxelSculptManager::HandleNodeDestroy(Node* node)
{
    if (mPendingTarget == static_cast<Voxel3D*>(node))
    {
        mPendingChanges.clear();
        mModifiedSet.clear();
        mStrokeActive = false;
        mPendingTarget = nullptr;
    }
}

void VoxelSculptManager::UpdateHover()
{
    mHoverValid = false;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != Voxel3D::GetStaticType())
        return;

    Voxel3D* voxel = static_cast<Voxel3D*>(selected);

    if (!GetEditorState()->GetViewport3D()->ShouldHandleInput())
        return;

    Camera3D* camera = GetWorld(0)->GetActiveCamera();
    if (camera == nullptr)
        return;

    int32_t mouseX, mouseY;
    INP_GetMousePosition(mouseX, mouseY);

    glm::vec3 rayOrigin = camera->GetWorldPosition();
    glm::vec3 rayTarget = camera->ScreenToWorldPosition(mouseX, mouseY);
    glm::vec3 rayDir = glm::normalize(rayTarget - rayOrigin);

    glm::ivec3 hitVoxel, prevVoxel;
    if (RayMarchVoxel(voxel, rayOrigin, rayDir, hitVoxel, prevVoxel))
    {
        mHoverValid = true;
        mHoverVoxel = hitVoxel;
        mHoverPlaceVoxel = prevVoxel;
    }
}

bool VoxelSculptManager::RayMarchVoxel(Voxel3D* voxel,
                                        const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                        glm::ivec3& outHitVoxel, glm::ivec3& outPrevVoxel)
{
    glm::ivec3 dims = voxel->GetDimensions();
    glm::vec3 halfDims = glm::vec3(dims) * 0.5f;

    glm::vec3 nodePos = voxel->GetWorldPosition();
    glm::vec3 nodeScale = voxel->GetWorldScale();

    // AABB in world space
    glm::vec3 boundsMin = nodePos - halfDims * nodeScale;
    glm::vec3 boundsMax = nodePos + halfDims * nodeScale;

    // Ray-AABB intersection (slab method)
    float tMin = -1e30f;
    float tMax = 1e30f;

    for (int axis = 0; axis < 3; ++axis)
    {
        float d = rayDir[axis];
        float o = rayOrigin[axis];
        float bMin = boundsMin[axis];
        float bMax = boundsMax[axis];

        if (glm::abs(d) > 0.0001f)
        {
            float t1 = (bMin - o) / d;
            float t2 = (bMax - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            if (t1 > tMin) tMin = t1;
            if (t2 < tMax) tMax = t2;
        }
        else if (o < bMin || o > bMax)
        {
            return false;
        }
    }

    if (tMin > tMax)
        return false;

    float maxDist = 200.0f;
    if (tMin > maxDist)
        return false;

    float startT = glm::max(0.0f, tMin);
    float endT = glm::min(tMax, maxDist);
    float step = 0.5f;
    int maxIter = 500;

    glm::ivec3 prevVoxelCoord(-1, -1, -1);
    bool hasPrev = false;

    for (int iter = 0; iter < maxIter; ++iter)
    {
        float t = startT + iter * step;
        if (t > endT) break;

        glm::vec3 worldPos = rayOrigin + rayDir * t;

        // Convert to voxel coordinates
        glm::vec3 localPos = (worldPos - nodePos) / nodeScale;
        glm::ivec3 voxelCoord = glm::ivec3(glm::floor(localPos + halfDims));

        // Bounds check
        if (voxelCoord.x >= 0 && voxelCoord.x < dims.x &&
            voxelCoord.y >= 0 && voxelCoord.y < dims.y &&
            voxelCoord.z >= 0 && voxelCoord.z < dims.z)
        {
            uint8_t val = voxel->GetVoxel(voxelCoord.x, voxelCoord.y, voxelCoord.z);

            if (val > 0)
            {
                outHitVoxel = voxelCoord;
                outPrevVoxel = hasPrev ? prevVoxelCoord : voxelCoord;
                return true;
            }

            if (!hasPrev || prevVoxelCoord != voxelCoord)
            {
                prevVoxelCoord = voxelCoord;
                hasPrev = true;
            }
        }
    }

    return false;
}

void VoxelSculptManager::DrawCursor()
{
    if (!mHoverValid)
        return;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != Voxel3D::GetStaticType())
        return;

    Voxel3D* voxel = static_cast<Voxel3D*>(selected);
    glm::ivec3 dims = voxel->GetDimensions();
    glm::vec3 halfDims = glm::vec3(dims) * 0.5f;
    glm::vec3 nodePos = voxel->GetWorldPosition();
    glm::vec3 nodeScale = voxel->GetWorldScale();

    // Choose which voxel to highlight based on mode
    glm::ivec3 target = (mOptions.mMode == VoxelSculptMode::Add) ? mHoverPlaceVoxel : mHoverVoxel;

    // Color based on mode
    glm::vec4 color;
    switch (mOptions.mMode)
    {
        case VoxelSculptMode::Add:    color = glm::vec4(0.2f, 0.9f, 0.2f, 0.5f); break;
        case VoxelSculptMode::Remove: color = glm::vec4(0.9f, 0.2f, 0.2f, 0.5f); break;
        case VoxelSculptMode::Paint:  color = glm::vec4(0.2f, 0.4f, 0.9f, 0.5f); break;
        default: color = glm::vec4(1.0f); break;
    }

    // Convert voxel coord to world position (center of voxel mesh)
    glm::vec3 worldPos = nodePos + (glm::vec3(target) - halfDims) * nodeScale;

    // Use DebugDraw with SM_Cube
    float brushScale = static_cast<float>(mOptions.mBrushRadius);
    glm::vec3 scale = nodeScale * brushScale;

    DebugDraw cursorDraw;
    cursorDraw.mMesh = LoadAsset<StaticMesh>("SM_Cube");
    cursorDraw.mTransform = glm::translate(glm::mat4(1.0f), worldPos) *
                            glm::scale(glm::mat4(1.0f), scale);
    cursorDraw.mColor = color;
    cursorDraw.mLife = 0.0f;
    Renderer::Get()->AddDebugDraw(cursorDraw);
}

void VoxelSculptManager::UpdateStroke()
{
    if (!GetEditorState()->GetViewport3D()->ShouldHandleInput())
        return;

    Node* selected = GetEditorState()->GetSelectedNode();
    if (selected == nullptr || selected->GetType() != Voxel3D::GetStaticType())
        return;

    Voxel3D* voxel = static_cast<Voxel3D*>(selected);

    if (IsMouseButtonJustDown(MOUSE_LEFT) && mHoverValid)
    {
        mStrokeActive = true;
        mPendingTarget = voxel;
        mPendingChanges.clear();
        mModifiedSet.clear();

        glm::ivec3 center = (mOptions.mMode == VoxelSculptMode::Add) ? mHoverPlaceVoxel : mHoverVoxel;
        ApplyBrush(voxel, center);
    }
    else if (IsMouseButtonDown(MOUSE_LEFT) && mStrokeActive && mHoverValid && mPendingTarget == voxel)
    {
        glm::ivec3 center = (mOptions.mMode == VoxelSculptMode::Add) ? mHoverPlaceVoxel : mHoverVoxel;
        ApplyBrush(voxel, center);
    }

    if (IsMouseButtonJustUp(MOUSE_LEFT) && mStrokeActive)
    {
        CommitStroke();
    }
}

void VoxelSculptManager::ApplyBrush(Voxel3D* voxel, glm::ivec3 center)
{
    glm::ivec3 dims = voxel->GetDimensions();
    int32_t r = mOptions.mBrushRadius;

    for (int32_t bx = -r + 1; bx <= r - 1; ++bx)
    {
        for (int32_t by = -r + 1; by <= r - 1; ++by)
        {
            for (int32_t bz = -r + 1; bz <= r - 1; ++bz)
            {
                float dist = glm::sqrt(float(bx*bx + by*by + bz*bz));
                if (dist >= float(r)) continue;

                int32_t vx = center.x + bx;
                int32_t vy = center.y + by;
                int32_t vz = center.z + bz;

                if (vx < 0 || vx >= dims.x || vy < 0 || vy >= dims.y || vz < 0 || vz >= dims.z)
                    continue;

                uint64_t key = VoxelKey(vx, vy, vz);
                if (mModifiedSet.count(key) > 0)
                    continue;

                uint8_t oldVal = voxel->GetVoxel(vx, vy, vz);
                uint8_t newVal = oldVal;

                switch (mOptions.mMode)
                {
                    case VoxelSculptMode::Add:
                        if (oldVal == 0) newVal = mOptions.mMaterialId;
                        break;
                    case VoxelSculptMode::Remove:
                        if (oldVal > 0) newVal = 0;
                        break;
                    case VoxelSculptMode::Paint:
                        if (oldVal > 0) newVal = mOptions.mMaterialId;
                        break;
                    default:
                        break;
                }

                if (newVal != oldVal)
                {
                    voxel->SetVoxel(vx, vy, vz, newVal);
                    mModifiedSet.insert(key);

                    VoxelChange change;
                    change.x = vx;
                    change.y = vy;
                    change.z = vz;
                    change.oldValue = oldVal;
                    change.newValue = newVal;
                    mPendingChanges.push_back(change);
                }
            }
        }
    }
}

void VoxelSculptManager::CommitStroke()
{
    if (!mPendingChanges.empty() && mPendingTarget != nullptr)
    {
        ActionManager::Get()->EXE_SetVoxels(mPendingTarget, mPendingChanges);
    }

    mPendingChanges.clear();
    mModifiedSet.clear();
    mStrokeActive = false;
    mPendingTarget = nullptr;
}

#endif

#include "Nodes/3D/Terrain3d.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Asset.h"
#include "Log.h"
#include "Assets/MaterialLite.h"
#include "Assets/Texture.h"
#include "Maths.h"
#include "Graphics/Graphics.h"

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>

#include <algorithm>
#include <cmath>

FORCE_LINK_DEF(Terrain3D);
DEFINE_NODE(Terrain3D, Mesh3D);

bool Terrain3D::HandlePropChange(Datum* datum, uint32_t index, const void* newValue)
{
    Property* prop = static_cast<Property*>(datum);
    OCT_ASSERT(prop != nullptr);
    Terrain3D* terrain = static_cast<Terrain3D*>(prop->mOwner);

    // When the heightmap texture property changes, import it automatically
    if (prop->mName == "Heightmap Texture")
    {
        Texture* tex = terrain->mHeightmapTexture.Get<Texture>();
        if (tex != nullptr)
        {
            terrain->SetHeightFromTexture(tex);
        }
    }

    // Snap position to grid when snap grid size is set
    if (terrain->mSnapGridSize > 0.0f)
    {
        glm::vec3 pos = terrain->GetWorldPosition();
        float snap = terrain->mSnapGridSize;
        pos.x = std::round(pos.x / snap) * snap;
        pos.z = std::round(pos.z / snap) * snap;
        terrain->SetWorldPosition(pos);
    }

    terrain->MarkDirty();
    return false;
}

Terrain3D::Terrain3D()
{
    mName = "Terrain";

    // Initialize bounds from default world size
    mBounds.mCenter = glm::vec3(0.0f);
    mBounds.mRadius = glm::length(glm::vec3(mWorldWidth, 0.0f, mWorldDepth)) * 0.5f;

    MarkDirty();
}

Terrain3D::~Terrain3D()
{

}

const char* Terrain3D::GetTypeName() const
{
    return "Terrain";
}

void Terrain3D::GatherProperties(std::vector<Property>& outProps)
{
    Mesh3D::GatherProperties(outProps);

    {
        SCOPED_CATEGORY("Terrain");
        outProps.push_back(Property(DatumType::Integer, "Resolution X", this, &mResolutionX, 1, HandlePropChange));
        outProps.push_back(Property(DatumType::Integer, "Resolution Z", this, &mResolutionZ, 1, HandlePropChange));
        outProps.push_back(Property(DatumType::Float, "World Width", this, &mWorldWidth, 1, HandlePropChange));
        outProps.push_back(Property(DatumType::Float, "World Depth", this, &mWorldDepth, 1, HandlePropChange));
        outProps.push_back(Property(DatumType::Float, "Height Scale", this, &mHeightScale, 1, HandlePropChange));
        outProps.push_back(Property(DatumType::Float, "Snap Grid", this, &mSnapGridSize, 1, HandlePropChange));
        outProps.push_back(Property(DatumType::Bool, "Use Material Slots", this, &mUseMaterialSlots, 1, HandlePropChange));
        outProps.push_back(Property(DatumType::Asset, "Heightmap Texture", this, &mHeightmapTexture, 1, HandlePropChange, int32_t(Texture::GetStaticType())));
    }
}

void Terrain3D::Create()
{
    Mesh3D::Create();

    InitHeightmap();

    GFX_CreateTerrain3DResource(this);

    // Build the mesh and collision immediately so bounds and raycasts work on first frame
    if (mMeshDirty)
    {
        RebuildMeshInternal();
        RecreateCollisionShape();
    }

    // Enable collision by default so physics raycasts (editor sculpting, NavMesh) work
    EnableCollision(true);

    // Create default material with Lit shading for proper lighting
    mDefaultMaterial = MaterialLite::New(LoadAsset<MaterialLite>("M_Default"));
    if (MaterialLite* mat = mDefaultMaterial.Get<MaterialLite>())
    {
        mat->SetShadingModel(ShadingModel::Lit);
        mat->SetVertexColorMode(VertexColorMode::Modulate);
    }

    MarkDirty();
}

void Terrain3D::InitHeightmap()
{
    size_t totalVerts = (size_t)mResolutionX * mResolutionZ;

    if (mHeightmap.size() != totalVerts)
    {
        mHeightmap.assign(totalVerts, 0.0f);
    }

    if (mSplatmap.size() != totalVerts)
    {
        mSplatmap.assign(totalVerts, 0x000000FF); // Default: full weight on slot 0 (R=255)
    }
}

void Terrain3D::Destroy()
{
    DestroyTriangleCollisionData();
    GFX_DestroyTerrain3DResource(this);
    Mesh3D::Destroy();
}

void Terrain3D::Copy(Node* srcNode, bool recurse)
{
    Mesh3D::Copy(srcNode, recurse);

    if (srcNode->GetType() != Terrain3D::GetStaticType())
        return;

    Terrain3D* src = static_cast<Terrain3D*>(srcNode);

    mResolutionX = src->mResolutionX;
    mResolutionZ = src->mResolutionZ;
    mWorldWidth = src->mWorldWidth;
    mWorldDepth = src->mWorldDepth;
    mHeightScale = src->mHeightScale;
    mSnapGridSize = src->mSnapGridSize;
    mHeightmap = src->mHeightmap;
    mSplatmap = src->mSplatmap;
    mUseMaterialSlots = src->mUseMaterialSlots;
    mHeightmapTexture = src->mHeightmapTexture;

    for (int32_t i = 0; i < MAX_MATERIAL_SLOTS; ++i)
    {
        mMaterialSlots[i] = src->mMaterialSlots[i];
    }

    MarkDirty();
}

Terrain3DResource* Terrain3D::GetResource()
{
    return &mResource;
}

void Terrain3D::Tick(float deltaTime)
{
    Mesh3D::Tick(deltaTime);
    TickCommon(deltaTime);
}

void Terrain3D::EditorTick(float deltaTime)
{
    Mesh3D::EditorTick(deltaTime);
    TickCommon(deltaTime);
}

void Terrain3D::TickCommon(float deltaTime)
{
    if (mMeshDirty)
    {
        RebuildMeshInternal();
        RecreateCollisionShape();
    }

    UploadMeshData();
}

void Terrain3D::SaveStream(Stream& stream, Platform platform)
{
    Mesh3D::SaveStream(stream, platform);

    stream.WriteInt32(mResolutionX);
    stream.WriteInt32(mResolutionZ);
    stream.WriteFloat(mWorldWidth);
    stream.WriteFloat(mWorldDepth);
    stream.WriteFloat(mHeightScale);
    stream.WriteFloat(mSnapGridSize);
    stream.WriteBool(mUseMaterialSlots);

    // Write heightmap data (per-float for endian safety on GCN/Wii)
    uint32_t heightmapSize = static_cast<uint32_t>(mHeightmap.size());
    stream.WriteUint32(heightmapSize);
    for (uint32_t i = 0; i < heightmapSize; ++i)
    {
        stream.WriteFloat(mHeightmap[i]);
    }

    // Write splatmap data (per-uint32 for endian safety on GCN/Wii)
    uint32_t splatmapSize = static_cast<uint32_t>(mSplatmap.size());
    stream.WriteUint32(splatmapSize);
    for (uint32_t i = 0; i < splatmapSize; ++i)
    {
        stream.WriteUint32(mSplatmap[i]);
    }

    // Write material slot references
    for (int32_t i = 0; i < MAX_MATERIAL_SLOTS; ++i)
    {
        stream.WriteAsset(mMaterialSlots[i]);
    }

    // Write heightmap texture reference
    stream.WriteAsset(mHeightmapTexture);
}

void Terrain3D::LoadStream(Stream& stream, Platform platform, uint32_t version)
{
    Mesh3D::LoadStream(stream, platform, version);

    if (version >= ASSET_VERSION_TERRAIN3D)
    {
        mResolutionX = stream.ReadInt32();
        mResolutionZ = stream.ReadInt32();
        mWorldWidth = stream.ReadFloat();
        mWorldDepth = stream.ReadFloat();
        mHeightScale = stream.ReadFloat();
        mSnapGridSize = stream.ReadFloat();

        if (version >= ASSET_VERSION_TERRAIN3D_MATSLOTS)
        {
            mUseMaterialSlots = stream.ReadBool();
        }

        // Read heightmap data (per-float for endian safety on GCN/Wii)
        uint32_t heightmapSize = stream.ReadUint32();
        mHeightmap.resize(heightmapSize);
        for (uint32_t i = 0; i < heightmapSize; ++i)
        {
            mHeightmap[i] = stream.ReadFloat();
        }

        // Read splatmap data (per-uint32 for endian safety on GCN/Wii)
        uint32_t splatmapSize = stream.ReadUint32();
        mSplatmap.resize(splatmapSize);
        for (uint32_t i = 0; i < splatmapSize; ++i)
        {
            mSplatmap[i] = stream.ReadUint32();
        }

        // Read material slot references
        for (int32_t i = 0; i < MAX_MATERIAL_SLOTS; ++i)
        {
            stream.ReadAsset(mMaterialSlots[i]);
        }

        // Read heightmap texture reference
        stream.ReadAsset(mHeightmapTexture);
    }

    MarkDirty();
}

bool Terrain3D::IsStaticMesh3D() const
{
    return false;
}

bool Terrain3D::IsSkeletalMesh3D() const
{
    return false;
}

Material* Terrain3D::GetMaterial()
{
    Material* mat = mMaterialOverride.Get<Material>();

    if (mat == nullptr)
    {
        mat = mDefaultMaterial.Get<Material>();
    }

    return mat;
}

void Terrain3D::Render()
{
    GFX_DrawTerrain3D(this);
}

Bounds Terrain3D::GetLocalBounds() const
{
    return mBounds;
}

void Terrain3D::SetHeight(int32_t x, int32_t z, float height)
{
    if (x >= 0 && x < mResolutionX && z >= 0 && z < mResolutionZ)
    {
        mHeightmap[z * mResolutionX + x] = height;
        MarkDirty();
    }
}

float Terrain3D::GetHeight(int32_t x, int32_t z) const
{
    if (x >= 0 && x < mResolutionX && z >= 0 && z < mResolutionZ)
    {
        return mHeightmap[z * mResolutionX + x];
    }
    return 0.0f;
}

float Terrain3D::GetHeightAtWorldPos(float worldX, float worldZ) const
{
    if (mResolutionX < 2 || mResolutionZ < 2)
        return 0.0f;

    float stepX = mWorldWidth / (mResolutionX - 1);
    float stepZ = mWorldDepth / (mResolutionZ - 1);
    float fx = (worldX + mWorldWidth * 0.5f) / stepX;
    float fz = (worldZ + mWorldDepth * 0.5f) / stepZ;

    int ix = (int)std::floor(fx);
    int iz = (int)std::floor(fz);
    float tx = fx - ix;
    float tz = fz - iz;

    // Clamp to valid interpolation range
    ix = std::max(0, std::min(ix, mResolutionX - 2));
    iz = std::max(0, std::min(iz, mResolutionZ - 2));

    // Bilinear interpolation
    float h00 = GetHeight(ix, iz);
    float h10 = GetHeight(ix + 1, iz);
    float h01 = GetHeight(ix, iz + 1);
    float h11 = GetHeight(ix + 1, iz + 1);

    float h0 = h00 + (h10 - h00) * tx;
    float h1 = h01 + (h11 - h01) * tx;

    return (h0 + (h1 - h0) * tz) * mHeightScale;
}

void Terrain3D::SetHeightFromTexture(Texture* texture)
{
    if (texture == nullptr)
        return;

    uint32_t texW = texture->GetWidth();
    uint32_t texH = texture->GetHeight();

    if (texW == 0 || texH == 0)
        return;

    // Texture pixels are RGBA8, 4 bytes per pixel.
    // Note: mPixels is only available in EDITOR builds (cleared after GPU upload in runtime).
    const uint32_t bpp = 4; // RGBA8

    if (texture->GetPixels().empty())
    {
        LogWarning("SetHeightFromTexture: texture has no CPU-side pixel data");
        return;
    }

    for (int32_t iz = 0; iz < mResolutionZ; ++iz)
    {
        for (int32_t ix = 0; ix < mResolutionX; ++ix)
        {
            float u = (float)ix / (mResolutionX - 1);
            float v = (float)iz / (mResolutionZ - 1);

            uint32_t px = std::min((uint32_t)(u * (texW - 1)), texW - 1);
            uint32_t py = std::min((uint32_t)(v * (texH - 1)), texH - 1);
            uint32_t pixelIdx = (py * texW + px) * bpp;

            if (pixelIdx < texture->GetPixels().size())
            {
                // Sample the R channel as normalized height [0, 1]
                float heightNorm = texture->GetPixels()[pixelIdx] / 255.0f;
                mHeightmap[iz * mResolutionX + ix] = heightNorm;
            }
        }
    }

    MarkDirty();
}

void Terrain3D::FlattenAll(float height)
{
    std::fill(mHeightmap.begin(), mHeightmap.end(), height);
    MarkDirty();
}

void Terrain3D::Resize(int32_t resX, int32_t resZ, float worldW, float worldD)
{
    mResolutionX = std::max(resX, 2);
    mResolutionZ = std::max(resZ, 2);
    mWorldWidth = worldW;
    mWorldDepth = worldD;

    // Force reallocate at the new resolution
    size_t totalVerts = (size_t)mResolutionX * mResolutionZ;
    mHeightmap.assign(totalVerts, 0.0f);
    mSplatmap.assign(totalVerts, 0x000000FF);

    MarkDirty();
}

void Terrain3D::SetMaterialWeight(int32_t x, int32_t z, int32_t slot, float weight)
{
    if (x < 0 || x >= mResolutionX || z < 0 || z >= mResolutionZ || slot < 0 || slot >= MAX_MATERIAL_SLOTS)
        return;

    size_t idx = z * mResolutionX + x;
    uint32_t packed = mSplatmap[idx];

    // Unpack RGBA bytes
    uint8_t weights[4];
    weights[0] = (packed >> 0) & 0xFF;
    weights[1] = (packed >> 8) & 0xFF;
    weights[2] = (packed >> 16) & 0xFF;
    weights[3] = (packed >> 24) & 0xFF;

    weights[slot] = (uint8_t)(std::max(0.0f, std::min(1.0f, weight)) * 255.0f);

    mSplatmap[idx] = weights[0] | (weights[1] << 8) | (weights[2] << 16) | (weights[3] << 24);
    MarkDirty();
}

float Terrain3D::GetMaterialWeight(int32_t x, int32_t z, int32_t slot) const
{
    if (x < 0 || x >= mResolutionX || z < 0 || z >= mResolutionZ || slot < 0 || slot >= MAX_MATERIAL_SLOTS)
        return 0.0f;

    size_t idx = z * mResolutionX + x;
    uint32_t packed = mSplatmap[idx];

    uint8_t weights[4];
    weights[0] = (packed >> 0) & 0xFF;
    weights[1] = (packed >> 8) & 0xFF;
    weights[2] = (packed >> 16) & 0xFF;
    weights[3] = (packed >> 24) & 0xFF;

    return weights[slot] / 255.0f;
}

void Terrain3D::SetMaterialSlot(int32_t slot, Material* mat)
{
    if (slot >= 0 && slot < MAX_MATERIAL_SLOTS)
    {
        mMaterialSlots[slot] = mat;
    }
}

Material* Terrain3D::GetMaterialSlot(int32_t slot) const
{
    if (slot >= 0 && slot < MAX_MATERIAL_SLOTS)
    {
        return mMaterialSlots[slot].Get<Material>();
    }
    return nullptr;
}

void Terrain3D::MarkDirty()
{
    mMeshDirty = true;
    for (int i = 0; i < MAX_FRAMES; ++i)
    {
        mUploadDirty[i] = true;
    }
}

void Terrain3D::RebuildMesh()
{
    RebuildMeshInternal();
    RecreateCollisionShape();

    // Mark upload dirty so GPU buffers get updated
    for (int i = 0; i < MAX_FRAMES; ++i)
    {
        mUploadDirty[i] = true;
    }
}

void Terrain3D::RebuildMeshInternal()
{
    if (mHeightmap.empty() || mResolutionX < 2 || mResolutionZ < 2)
    {
        mMeshDirty = false;
        return;
    }

    float stepX = mWorldWidth / (mResolutionX - 1);
    float stepZ = mWorldDepth / (mResolutionZ - 1);
    float halfW = mWorldWidth * 0.5f;
    float halfD = mWorldDepth * 0.5f;

    // Generate vertices
    mVertices.clear();
    mVertices.reserve(mResolutionX * mResolutionZ);

    for (int32_t iz = 0; iz < mResolutionZ; ++iz)
    {
        for (int32_t ix = 0; ix < mResolutionX; ++ix)
        {
            size_t idx = iz * mResolutionX + ix;
            VertexColor v;

            // Position (centered on origin)
            v.mPosition.x = ix * stepX - halfW;
            v.mPosition.y = mHeightmap[idx] * mHeightScale;
            v.mPosition.z = iz * stepZ - halfD;

            // Normal via central differences
            float hL = (ix > 0) ? mHeightmap[iz * mResolutionX + (ix - 1)] : mHeightmap[idx];
            float hR = (ix < mResolutionX - 1) ? mHeightmap[iz * mResolutionX + (ix + 1)] : mHeightmap[idx];
            float hD = (iz > 0) ? mHeightmap[(iz - 1) * mResolutionX + ix] : mHeightmap[idx];
            float hU = (iz < mResolutionZ - 1) ? mHeightmap[(iz + 1) * mResolutionX + ix] : mHeightmap[idx];

            glm::vec3 normal = glm::normalize(glm::vec3(
                (hL - hR) * mHeightScale,
                2.0f * stepX,
                (hD - hU) * mHeightScale
            ));
            v.mNormal = normal;

            // UVs
            v.mTexcoord0 = glm::vec2(
                (float)ix / (mResolutionX - 1),
                (float)iz / (mResolutionZ - 1)
            );
            v.mTexcoord1 = glm::vec2(0.0f);

            // Color from splatmap (material weights) or white if material slots disabled
            if (mUseMaterialSlots)
                v.mColor = (idx < mSplatmap.size()) ? mSplatmap[idx] : 0x000000FF;
            else
                v.mColor = 0xFFFFFFFF; // White — no tinting

            mVertices.push_back(v);
        }
    }

    // Generate indices (2 triangles per quad cell)
    mIndices.clear();
    mIndices.reserve((mResolutionX - 1) * (mResolutionZ - 1) * 6);

    for (int32_t iz = 0; iz < mResolutionZ - 1; ++iz)
    {
        for (int32_t ix = 0; ix < mResolutionX - 1; ++ix)
        {
            IndexType topLeft     = iz * mResolutionX + ix;
            IndexType topRight    = iz * mResolutionX + ix + 1;
            IndexType bottomLeft  = (iz + 1) * mResolutionX + ix;
            IndexType bottomRight = (iz + 1) * mResolutionX + ix + 1;

            // Triangle 1
            mIndices.push_back(topLeft);
            mIndices.push_back(bottomLeft);
            mIndices.push_back(topRight);

            // Triangle 2
            mIndices.push_back(topRight);
            mIndices.push_back(bottomLeft);
            mIndices.push_back(bottomRight);
        }
    }

    mNumVertices = static_cast<uint32_t>(mVertices.size());
    mNumIndices = static_cast<uint32_t>(mIndices.size());

    UpdateBounds();
    mMeshDirty = false;
}

void Terrain3D::UploadMeshData()
{
    uint32_t frameIndex = Renderer::Get()->GetFrameIndex();

    if (mUploadDirty[frameIndex] && mNumVertices > 0 && mNumIndices > 0)
    {
        GFX_UpdateTerrain3DResource(this, mVertices, mIndices);
        mUploadDirty[frameIndex] = false;
    }
}

void Terrain3D::UpdateBounds()
{
    if (mVertices.empty())
    {
        mBounds.mCenter = glm::vec3(0.0f);
        mBounds.mRadius = 0.0f;
        return;
    }

    glm::vec3 minPos = mVertices[0].mPosition;
    glm::vec3 maxPos = mVertices[0].mPosition;

    for (const auto& v : mVertices)
    {
        minPos = glm::min(minPos, v.mPosition);
        maxPos = glm::max(maxPos, v.mPosition);
    }

    mBounds.mCenter = (minPos + maxPos) * 0.5f;
    mBounds.mRadius = glm::length(maxPos - minPos) * 0.5f;
}

void Terrain3D::RecreateCollisionShape()
{
    DestroyTriangleCollisionData();

    if (mVertices.empty() || mIndices.empty())
    {
        SetCollisionShape(GetEmptyCollisionShape());
        return;
    }

    // Build collision vertex/index arrays from the generated mesh
    mCollisionVertices.resize(mVertices.size());
    for (size_t i = 0; i < mVertices.size(); ++i)
    {
        mCollisionVertices[i] = mVertices[i].mPosition;
    }

    mCollisionIndices.resize(mIndices.size());
    for (size_t i = 0; i < mIndices.size(); ++i)
    {
        mCollisionIndices[i] = static_cast<int32_t>(mIndices[i]);
    }

    mTriangleIndexVertexArray = new btTriangleIndexVertexArray();

    btIndexedMesh mesh;
    mesh.m_numTriangles = static_cast<int>(mCollisionIndices.size() / 3);
    mesh.m_triangleIndexBase = reinterpret_cast<const unsigned char*>(mCollisionIndices.data());
    mesh.m_triangleIndexStride = sizeof(int32_t) * 3;
    mesh.m_numVertices = static_cast<int>(mCollisionVertices.size());
    mesh.m_vertexBase = reinterpret_cast<const unsigned char*>(mCollisionVertices.data());
    mesh.m_vertexStride = sizeof(glm::vec3);

    mTriangleIndexVertexArray->addIndexedMesh(mesh, PHY_INTEGER);

    btBvhTriangleMeshShape* triShape = new btBvhTriangleMeshShape(mTriangleIndexVertexArray, true);

    mTriangleInfoMap = new btTriangleInfoMap();
    btGenerateInternalEdgeInfo(triShape, mTriangleInfoMap);

    SetCollisionShape(triShape);
}

void Terrain3D::DestroyTriangleCollisionData()
{
    if (mTriangleInfoMap != nullptr)
    {
        delete mTriangleInfoMap;
        mTriangleInfoMap = nullptr;
    }

    if (mTriangleIndexVertexArray != nullptr)
    {
        delete mTriangleIndexVertexArray;
        mTriangleIndexVertexArray = nullptr;
    }

    mCollisionVertices.clear();
    mCollisionIndices.clear();
}

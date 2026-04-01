#include "Nodes/3D/Voxel3d.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Log.h"

#include "Assets/MaterialLite.h"

#include "Graphics/Graphics.h"

// Disable warnings for PolyVox external library
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4267) // conversion from 'size_t' to 'type', possible loss of data
#endif

#include <PolyVox/RawVolume.h>
#include <PolyVox/Region.h>
#include <PolyVox/CubicSurfaceExtractor.h>
#include <PolyVox/Mesh.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

FORCE_LINK_DEF(Voxel3D);
DEFINE_NODE(Voxel3D, Mesh3D);

bool Voxel3D::HandlePropChange(Datum* datum, uint32_t index, const void* newValue)
{
    Property* prop = static_cast<Property*>(datum);
    OCT_ASSERT(prop != nullptr);
    Voxel3D* voxelComp = static_cast<Voxel3D*>(prop->mOwner);

    voxelComp->MarkDirty();

    return false;
}

Voxel3D::Voxel3D()
{
    mName = "Voxel";

    // Initialize bounds centered at origin to cover default volume size
    mBounds.mCenter = glm::vec3(0.0f);
    mBounds.mRadius = glm::length(glm::vec3(mDimensions)) * 0.5f;

    MarkDirty();
}

Voxel3D::~Voxel3D()
{

}

const char* Voxel3D::GetTypeName() const
{
    return "Voxel";
}

void Voxel3D::GatherProperties(std::vector<Property>& outProps)
{
    Mesh3D::GatherProperties(outProps);

    SCOPED_CATEGORY("Voxel");

    outProps.push_back(Property(DatumType::Integer, "Dimension X", this, &mDimensions.x, 1, HandlePropChange));
    outProps.push_back(Property(DatumType::Integer, "Dimension Y", this, &mDimensions.y, 1, HandlePropChange));
    outProps.push_back(Property(DatumType::Integer, "Dimension Z", this, &mDimensions.z, 1, HandlePropChange));
}

void Voxel3D::Create()
{
    Mesh3D::Create();

    // Create the volume
    bool newVolume = (mVolume == nullptr);
    if (newVolume)
    {
        PolyVox::Region region(
            PolyVox::Vector3DInt32(0, 0, 0),
            PolyVox::Vector3DInt32(mDimensions.x - 1, mDimensions.y - 1, mDimensions.z - 1)
        );
        mVolume = new PolyVox::RawVolume<VoxelType>(region);
    }

    // Decompress loaded voxel data if any
    DecompressVoxelData();

    // If this is a new volume (not loaded from scene), fill with a test cube
    if (newVolume && mCompressedData.empty())
    {
        // Fill a smaller cube in the center as a test
        int32_t margin = 4;
        FillRegion(margin, margin, margin,
                   mDimensions.x - margin - 1,
                   mDimensions.y - margin - 1,
                   mDimensions.z - margin - 1, 1);
    }

    GFX_CreateVoxel3DResource(this);

    // Build the mesh immediately so bounds are valid for first frame
    if (mMeshDirty)
    {
        RebuildMeshInternal();
    }

    // Create default material with Lit shading for proper lighting
    mDefaultMaterial = MaterialLite::New(LoadAsset<MaterialLite>("M_Default"));
    if (MaterialLite* mat = mDefaultMaterial.Get<MaterialLite>())
    {
        mat->SetShadingModel(ShadingModel::Lit);
        mat->SetVertexColorMode(VertexColorMode::Modulate);
    }

    MarkDirty();
}

void Voxel3D::Destroy()
{
    GFX_DestroyVoxel3DResource(this);

    if (mVolume != nullptr)
    {
        delete mVolume;
        mVolume = nullptr;
    }

    Mesh3D::Destroy();
}

Voxel3DResource* Voxel3D::GetResource()
{
    return &mResource;
}

void Voxel3D::Tick(float deltaTime)
{
    Mesh3D::Tick(deltaTime);
    TickCommon(deltaTime);
}

void Voxel3D::EditorTick(float deltaTime)
{
    Mesh3D::EditorTick(deltaTime);
    TickCommon(deltaTime);
}

void Voxel3D::TickCommon(float deltaTime)
{
    if (mMeshDirty)
    {
        RebuildMeshInternal();
        RecreateCollisionShape();
    }

    UploadMeshData();
}

void Voxel3D::SaveStream(Stream& stream, Platform platform)
{
    Mesh3D::SaveStream(stream, platform);

    // Write dimensions
    stream.WriteInt32(mDimensions.x);
    stream.WriteInt32(mDimensions.y);
    stream.WriteInt32(mDimensions.z);

    // Write voxel data with RLE compression
    if (mVolume != nullptr)
    {
        std::vector<uint8_t> compressed;
        VoxelType lastVoxel = 0;
        uint32_t runLength = 0;

        const auto& region = mVolume->getEnclosingRegion();
        for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z)
        {
            for (int32_t y = region.getLowerY(); y <= region.getUpperY(); ++y)
            {
                for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x)
                {
                    VoxelType v = mVolume->getVoxel(x, y, z);
                    if (v == lastVoxel && runLength < 255)
                    {
                        ++runLength;
                    }
                    else
                    {
                        if (runLength > 0)
                        {
                            compressed.push_back(static_cast<uint8_t>(runLength));
                            compressed.push_back(lastVoxel);
                        }
                        lastVoxel = v;
                        runLength = 1;
                    }
                }
            }
        }
        // Flush final run
        if (runLength > 0)
        {
            compressed.push_back(static_cast<uint8_t>(runLength));
            compressed.push_back(lastVoxel);
        }

        stream.WriteUint32(static_cast<uint32_t>(compressed.size()));
        if (compressed.size() > 0)
        {
            stream.WriteBytes(compressed.data(), static_cast<uint32_t>(compressed.size()));
        }
    }
    else
    {
        stream.WriteUint32(0);
    }
}

void Voxel3D::LoadStream(Stream& stream, Platform platform, uint32_t version)
{
    Mesh3D::LoadStream(stream, platform, version);

    // Read dimensions
    mDimensions.x = stream.ReadInt32();
    mDimensions.y = stream.ReadInt32();
    mDimensions.z = stream.ReadInt32();

    // Read compressed voxel data
    uint32_t compressedSize = stream.ReadUint32();
    if (compressedSize > 0)
    {
        mCompressedData.resize(compressedSize);
        stream.ReadBytes(mCompressedData.data(), compressedSize);
    }

    MarkDirty();
}

bool Voxel3D::IsStaticMesh3D() const
{
    return false;
}

bool Voxel3D::IsSkeletalMesh3D() const
{
    return false;
}

Material* Voxel3D::GetMaterial()
{
    Material* mat = mMaterialOverride.Get<Material>();

    if (mat == nullptr)
    {
        mat = mDefaultMaterial.Get<Material>();
    }

    return mat;
}

void Voxel3D::Render()
{
    GFX_DrawVoxel3D(this);
}

void Voxel3D::SetVoxel(int32_t x, int32_t y, int32_t z, VoxelType value)
{
    if (mVolume != nullptr)
    {
        const auto& region = mVolume->getEnclosingRegion();
        if (region.containsPoint(x, y, z))
        {
            if (mVolume->getVoxel(x, y, z) != value)
            {
                mVolume->setVoxel(x, y, z, value);
                MarkDirty();
            }
        }
    }
}

VoxelType Voxel3D::GetVoxel(int32_t x, int32_t y, int32_t z) const
{
    if (mVolume != nullptr)
    {
        const auto& region = mVolume->getEnclosingRegion();
        if (region.containsPoint(x, y, z))
        {
            return mVolume->getVoxel(x, y, z);
        }
    }
    return 0;
}

void Voxel3D::Fill(VoxelType value)
{
    if (mVolume != nullptr)
    {
        const auto& region = mVolume->getEnclosingRegion();
        for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z)
        {
            for (int32_t y = region.getLowerY(); y <= region.getUpperY(); ++y)
            {
                for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x)
                {
                    mVolume->setVoxel(x, y, z, value);
                }
            }
        }
        MarkDirty();
    }
}

void Voxel3D::FillRegion(int32_t x0, int32_t y0, int32_t z0,
                          int32_t x1, int32_t y1, int32_t z1, VoxelType value)
{
    if (mVolume != nullptr)
    {
        const auto& region = mVolume->getEnclosingRegion();

        // Clamp to volume bounds
        x0 = glm::max(x0, region.getLowerX());
        y0 = glm::max(y0, region.getLowerY());
        z0 = glm::max(z0, region.getLowerZ());
        x1 = glm::min(x1, region.getUpperX());
        y1 = glm::min(y1, region.getUpperY());
        z1 = glm::min(z1, region.getUpperZ());

        for (int32_t z = z0; z <= z1; ++z)
        {
            for (int32_t y = y0; y <= y1; ++y)
            {
                for (int32_t x = x0; x <= x1; ++x)
                {
                    mVolume->setVoxel(x, y, z, value);
                }
            }
        }
        MarkDirty();
    }
}

void Voxel3D::MarkDirty()
{
    mMeshDirty = true;
    for (int i = 0; i < MAX_FRAMES; ++i)
    {
        mUploadDirty[i] = true;
    }
}

void Voxel3D::RebuildMesh()
{
    RebuildMeshInternal();
    RecreateCollisionShape();

    // Mark upload dirty so GPU buffers get updated
    for (int i = 0; i < MAX_FRAMES; ++i)
    {
        mUploadDirty[i] = true;
    }
}

void Voxel3D::SetDimensions(glm::ivec3 dims)
{
    if (mDimensions != dims)
    {
        mDimensions = dims;

        // Recreate volume if it exists
        if (mVolume != nullptr)
        {
            delete mVolume;
            PolyVox::Region region(
                PolyVox::Vector3DInt32(0, 0, 0),
                PolyVox::Vector3DInt32(mDimensions.x - 1, mDimensions.y - 1, mDimensions.z - 1)
            );
            mVolume = new PolyVox::RawVolume<VoxelType>(region);
        }

        MarkDirty();
    }
}

Bounds Voxel3D::GetLocalBounds() const
{
    return mBounds;
}

void Voxel3D::RebuildMeshInternal()
{
    if (mVolume == nullptr)
    {
        mMeshDirty = false;
        return;
    }

    // Extract cubic mesh from volume
    auto region = mVolume->getEnclosingRegion();
    auto mesh = PolyVox::extractCubicMesh(mVolume, region);
    auto decodedMesh = PolyVox::decodeMesh(mesh);

    // Convert to engine vertex format
    // Offset to center mesh around origin
    glm::vec3 centerOffset = glm::vec3(mDimensions) * 0.5f;

    // Build a temporary vertex array from the decoded mesh
    size_t numDecodedVerts = decodedMesh.getNoOfVertices();
    std::vector<VertexColor> decoded;
    decoded.reserve(numDecodedVerts);
    for (uint32_t i = 0; i < numDecodedVerts; ++i)
    {
        const auto& pv = decodedMesh.getVertex(i);
        VertexColor v;
        v.mPosition = glm::vec3(pv.position.getX(), pv.position.getY(), pv.position.getZ()) - centerOffset;
        v.mNormal = glm::vec3(0.0f);
        v.mTexcoord0 = glm::vec2(0.0f);
        v.mTexcoord1 = glm::vec2(0.0f);
        v.mColor = ColorFloat4ToUint32(MaterialIdToColor(pv.data));
        decoded.push_back(v);
    }

    // PolyVox's cubic extractor shares vertices across face boundaries.
    // Assigning face normals in-place causes the last triangle touching a shared
    // vertex to overwrite the normal, producing incorrect shading on adjacent faces.
    // Fix: unpack every triangle to 3 unique vertices (flat shading) so no vertex
    // is shared across faces. This gives each face a clean, correct normal.
    size_t numIndices = decodedMesh.getNoOfIndices();
    mVertices.clear();
    mVertices.reserve(numIndices); // 3 unique verts per triangle corner
    mIndices.clear();
    mIndices.reserve(numIndices);

    for (size_t i = 0; i + 2 < numIndices; i += 3)
    {
        VertexColor v0 = decoded[decodedMesh.getIndex(static_cast<uint32_t>(i))];
        VertexColor v1 = decoded[decodedMesh.getIndex(static_cast<uint32_t>(i + 1))];
        VertexColor v2 = decoded[decodedMesh.getIndex(static_cast<uint32_t>(i + 2))];

        glm::vec3 edge1 = v1.mPosition - v0.mPosition;
        glm::vec3 edge2 = v2.mPosition - v0.mPosition;
        glm::vec3 cross = glm::cross(edge1, edge2);
        float len = glm::length(cross);
        glm::vec3 n = (len > 1e-6f) ? (cross / len) : glm::vec3(0.0f, 1.0f, 0.0f);

        v0.mNormal = v1.mNormal = v2.mNormal = n;

        IndexType base = static_cast<IndexType>(mVertices.size());
        mVertices.push_back(v0);
        mVertices.push_back(v1);
        mVertices.push_back(v2);
        mIndices.push_back(base);
        mIndices.push_back(base + 1);
        mIndices.push_back(base + 2);
    }

    mNumVertices = static_cast<uint32_t>(mVertices.size());
    mNumIndices = static_cast<uint32_t>(mIndices.size());

    UpdateBounds();
    mMeshDirty = false;
}

void Voxel3D::UploadMeshData()
{
    uint32_t frameIndex = Renderer::Get()->GetFrameIndex();

    if (mUploadDirty[frameIndex] && mNumVertices > 0 && mNumIndices > 0)
    {
        GFX_UpdateVoxel3DResource(this, mVertices, mIndices);
        mUploadDirty[frameIndex] = false;
    }
}

void Voxel3D::UpdateBounds()
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

void Voxel3D::RecreateCollisionShape()
{
    // Simple bounding box collision for MVP
    glm::vec3 halfExtents = glm::vec3(mDimensions) * 0.5f;
    btBoxShape* boxShape = new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
    SetCollisionShape(boxShape);
}

void Voxel3D::DecompressVoxelData()
{
    if (mCompressedData.empty() || mVolume == nullptr)
    {
        return;
    }

    const auto& region = mVolume->getEnclosingRegion();
    size_t dataIndex = 0;

    int32_t x = region.getLowerX();
    int32_t y = region.getLowerY();
    int32_t z = region.getLowerZ();

    while (dataIndex + 1 < mCompressedData.size())
    {
        uint8_t runLength = mCompressedData[dataIndex++];
        VoxelType value = mCompressedData[dataIndex++];

        for (uint32_t i = 0; i < runLength; ++i)
        {
            if (z > region.getUpperZ())
            {
                break;
            }

            mVolume->setVoxel(x, y, z, value);

            ++x;
            if (x > region.getUpperX())
            {
                x = region.getLowerX();
                ++y;
                if (y > region.getUpperY())
                {
                    y = region.getLowerY();
                    ++z;
                }
            }
        }
    }

    mCompressedData.clear();
}

glm::vec4 Voxel3D::MaterialIdToColor(VoxelType materialId) const
{
    // Simple material ID to color mapping
    // 0 = air (shouldn't render)
    // 1-7 = basic colors
    switch (materialId)
    {
        case 0: return glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // Air
        case 1: return glm::vec4(0.8f, 0.8f, 0.8f, 1.0f); // White/Stone
        case 2: return glm::vec4(0.6f, 0.4f, 0.2f, 1.0f); // Brown/Dirt
        case 3: return glm::vec4(0.2f, 0.8f, 0.2f, 1.0f); // Green/Grass
        case 4: return glm::vec4(0.2f, 0.4f, 0.8f, 1.0f); // Blue/Water
        case 5: return glm::vec4(0.8f, 0.2f, 0.2f, 1.0f); // Red
        case 6: return glm::vec4(0.8f, 0.8f, 0.2f, 1.0f); // Yellow
        case 7: return glm::vec4(0.4f, 0.4f, 0.4f, 1.0f); // Gray
        default:
        {
            // Generate a color from material ID for IDs > 7
            float hue = (materialId * 0.618033988749895f);
            hue = hue - glm::floor(hue);
            // Simple HSV to RGB (saturation=0.7, value=0.9)
            float h = hue * 6.0f;
            float c = 0.9f * 0.7f;
            float x = c * (1.0f - glm::abs(glm::mod(h, 2.0f) - 1.0f));
            float m = 0.9f - c;
            glm::vec3 rgb;
            if (h < 1.0f) rgb = glm::vec3(c, x, 0.0f);
            else if (h < 2.0f) rgb = glm::vec3(x, c, 0.0f);
            else if (h < 3.0f) rgb = glm::vec3(0.0f, c, x);
            else if (h < 4.0f) rgb = glm::vec3(0.0f, x, c);
            else if (h < 5.0f) rgb = glm::vec3(x, 0.0f, c);
            else rgb = glm::vec3(c, 0.0f, x);
            return glm::vec4(rgb + m, 1.0f);
        }
    }
}

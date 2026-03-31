#pragma once

#include "Nodes/3D/Mesh3d.h"
#include "Vertex.h"

// Disable warnings for PolyVox external library
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4267) // conversion from 'size_t' to 'type', possible loss of data
#endif

#include <PolyVox/RawVolume.h>
#include <PolyVox/Region.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// Voxel type: 0 = air (empty), 1-255 = solid material IDs
using VoxelType = uint8_t;

class POLYPHASE_API Voxel3D : public Mesh3D
{
public:
    DECLARE_NODE(Voxel3D, Mesh3D);

    Voxel3D();
    ~Voxel3D();

    virtual const char* GetTypeName() const override;
    virtual void GatherProperties(std::vector<Property>& outProps) override;

    virtual void Create() override;
    virtual void Destroy() override;
    Voxel3DResource* GetResource();

    virtual void Tick(float deltaTime) override;
    virtual void EditorTick(float deltaTime) override;

    virtual void SaveStream(Stream& stream, Platform platform) override;
    virtual void LoadStream(Stream& stream, Platform platform, uint32_t version) override;

    virtual bool IsStaticMesh3D() const override;
    virtual bool IsSkeletalMesh3D() const override;
    virtual Material* GetMaterial() override;

    virtual void Render() override;

    // Voxel API
    void SetVoxel(int32_t x, int32_t y, int32_t z, VoxelType value);
    VoxelType GetVoxel(int32_t x, int32_t y, int32_t z) const;
    void Fill(VoxelType value);
    void FillRegion(int32_t x0, int32_t y0, int32_t z0,
                    int32_t x1, int32_t y1, int32_t z1, VoxelType value);

    // Mesh control
    void MarkDirty();
    void RebuildMesh();
    bool IsDirty() const { return mMeshDirty; }

    // Dimensions
    glm::ivec3 GetDimensions() const { return mDimensions; }
    void SetDimensions(glm::ivec3 dims);

    virtual Bounds GetLocalBounds() const override;

    uint32_t GetNumVertices() const { return mNumVertices; }
    uint32_t GetNumIndices() const { return mNumIndices; }
    const std::vector<VertexColor>& GetVertices() const { return mVertices; }
    const std::vector<IndexType>& GetIndices() const { return mIndices; }

protected:

    static bool HandlePropChange(Datum* datum, uint32_t index, const void* newValue);

    void TickCommon(float deltaTime);
    void RebuildMeshInternal();
    void UploadMeshData();
    void UpdateBounds();
    void RecreateCollisionShape();
    void DecompressVoxelData();
    glm::vec4 MaterialIdToColor(VoxelType materialId) const;

    // Volume data
    PolyVox::RawVolume<VoxelType>* mVolume = nullptr;
    glm::ivec3 mDimensions = { 32, 32, 32 };

    // Compressed voxel data (for serialization)
    std::vector<uint8_t> mCompressedData;

    // Generated mesh data (CPU side)
    std::vector<VertexColor> mVertices;
    std::vector<IndexType> mIndices;
    uint32_t mNumVertices = 0;
    uint32_t mNumIndices = 0;

    // Dirty tracking
    bool mMeshDirty = true;
    bool mUploadDirty[MAX_FRAMES] = {};

    // Bounds cache
    Bounds mBounds;

    // GPU resource
    Voxel3DResource mResource;

    // Default material
    MaterialRef mDefaultMaterial;
};

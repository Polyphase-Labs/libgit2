# Terrain3D in Polyphase Engine

## Overview

`Terrain3D` is a heightmap-based terrain node that generates a renderable mesh grid from per-vertex height data. It supports material painting via a splatmap, collision generation, NavMesh integration, heightmap texture import, grayscale brush masks, and terrain chunk snapping.

Key features:
- Configurable grid resolution and world-space dimensions
- Runtime height manipulation via C++ and Lua
- Import heightmap from grayscale textures
- 4-slot material painting with splatmap vertex colors
- Automatic triangle mesh collision (Bullet physics)
- NavMesh triangle contribution for pathfinding
- Snap grid for aligning terrain chunks edge-to-edge
- Editor sculpt tools: Raise, Lower, Flatten, Smooth, Paint Material
- Grayscale brush mask textures for custom brush shapes
- Scene persistence with full serialization
- Works on all platforms (Vulkan, GX, C3D) using MaterialLite

---

## How It Works

### Heightmap Storage

- **Data format**: `std::vector<float>` with one height value per vertex, stored row-major (Z-major)
- **Default resolution**: 128x128 vertices
- **Coordinate system**: Integer grid coordinates (x, z) starting from (0, 0)
- **Height values**: Raw floats multiplied by `mHeightScale` during mesh generation

### Mesh Generation Pipeline

1. When heights or splatmap are modified, the node is marked dirty
2. On the next tick, `RebuildMeshInternal()` is called
3. A regular grid of `resX * resZ` vertices is generated centered on the origin
4. Per-vertex normals are computed via central differences on the heightmap
5. UVs are mapped to [0,1] across the terrain surface
6. Vertex colors encode the splatmap (RGBA = 4 material slot weights)
7. Indices generate 2 triangles per quad cell
8. The mesh is uploaded to the GPU
9. Collision shape is rebuilt from the same triangle data

### Splatmap / Material Weights

Each vertex stores 4 material weights packed as RGBA in vertex color:
- R channel = Slot 0 weight
- G channel = Slot 1 weight
- B channel = Slot 2 weight
- A channel = Slot 3 weight

Values are 0-255 (0.0 to 1.0). When painting, weights are normalized so the active slot increases while others decrease proportionally.

### Brush Mask System

The editor (and Lua API) supports grayscale texture brush masks:
- Any RGBA texture works (R channel is sampled)
- White (255) = full brush effect, Black (0) = no effect
- The mask is mapped to the brush radius area (UV from brush center offset)
- When no mask is set, a radial cosine falloff is used instead

---

## C++ API

### Height Access

```cpp
void SetHeight(int32_t x, int32_t z, float height);
float GetHeight(int32_t x, int32_t z) const;
float GetHeightAtWorldPos(float worldX, float worldZ) const; // Bilinear interpolation
void SetHeightFromTexture(Texture* texture); // Import from grayscale image
void FlattenAll(float height = 0.0f);
void Resize(int32_t resX, int32_t resZ, float worldW, float worldD);
```

### Material Slots

```cpp
void SetMaterialSlot(int32_t slot, Material* mat); // 0-3, MaterialLite only
Material* GetMaterialSlot(int32_t slot) const;
void SetMaterialWeight(int32_t x, int32_t z, int32_t slot, float weight);
float GetMaterialWeight(int32_t x, int32_t z, int32_t slot) const;
```

### Mesh Control

```cpp
void MarkDirty();      // Mark for rebuild on next tick
void RebuildMesh();    // Force immediate rebuild
bool IsDirty() const;  // Check if rebuild pending
```

### Accessors

```cpp
int32_t GetResolutionX() const;
int32_t GetResolutionZ() const;
float GetWorldWidth() const;
float GetWorldDepth() const;
uint32_t GetNumVertices() const;
uint32_t GetNumIndices() const;
const std::vector<VertexColor>& GetVertices() const;
const std::vector<IndexType>& GetIndices() const;
```

---

## Lua API

### Example Usage

```lua
-- Create a terrain
local terrain = world:SpawnNode("Terrain3D")
terrain:SetPosition(Vec(0, 0, 0))

-- Set some heights
for x = 0, 127 do
    for z = 0, 127 do
        local h = math.sin(x * 0.1) * math.cos(z * 0.1) * 2.0
        terrain:SetHeight(x, z, h)
    end
end

-- Import a heightmap texture
local hmap = LoadAsset("T_Heightmap")
terrain:SetHeightFromTexture(hmap)

-- Paint material slot 1 in a region
for x = 20, 40 do
    for z = 20, 40 do
        terrain:SetMaterialWeight(x, z, 1, 1.0)
        terrain:SetMaterialWeight(x, z, 0, 0.0)
    end
end

-- Force rebuild
terrain:RebuildMesh()
```

### Method Reference

| Method | Arguments | Returns | Description |
|--------|-----------|---------|-------------|
| `SetHeight` | `x, z, height` | - | Set height at grid position |
| `GetHeight` | `x, z` | `float` | Get height at grid position |
| `GetHeightAtWorldPos` | `worldX, worldZ` | `float` | Bilinear interpolated height |
| `SetHeightFromTexture` | `texture` | - | Import grayscale heightmap |
| `FlattenAll` | `[height]` | - | Set all heights (default 0) |
| `Resize` | `resX, resZ, worldW, worldD` | - | Resize grid (clears data) |
| `GetResolution` | - | `x, z` | Get grid resolution |
| `GetWorldSize` | - | `w, d` | Get world dimensions |
| `GetHeightScale` | - | `float` | Get height multiplier |
| `SetHeightScale` | `scale` | - | Set height multiplier |
| `MarkDirty` | - | - | Mark for rebuild |
| `RebuildMesh` | - | - | Force immediate rebuild |
| `IsDirty` | - | `bool` | Check if rebuild pending |
| `SetMaterialSlot` | `slot, material` | - | Set material (0-3) |
| `GetMaterialSlot` | `slot` | `material` | Get material at slot |
| `SetMaterialWeight` | `x, z, slot, weight` | - | Set per-vertex weight |
| `GetMaterialWeight` | `x, z, slot` | `float` | Get per-vertex weight |
| `SetSnapGridSize` | `size` | - | Set snap grid (0=off) |
| `GetSnapGridSize` | - | `float` | Get snap grid size |

---

## Editor Tools

### Entering Terrain Sculpt Mode

1. Select a Terrain3D node in the scene
2. Click "Open In Terrain Sculpt" in the Properties inspector, or
3. Select "Terrain Sculpt" from the editor mode dropdown in the toolbar

### Sculpt Modes

| Mode | Color | Description |
|------|-------|-------------|
| Raise | Green | Increase height under brush |
| Lower | Red | Decrease height under brush |
| Flatten | Yellow | Blend heights toward target value |
| Smooth | Blue | Average heights with neighbors |
| Paint Material | Purple | Paint material slot weights |

### Brush Settings

- **Radius**: World-space brush radius (0.5 - 50)
- **Strength**: Effect intensity per frame (0.01 - 1.0)
- **Falloff**: Edge softness when no brush mask (0 = hard, 1 = soft cosine)
- **Brush Mask**: Optional grayscale texture for custom brush shape
- **Target Height**: Only for Flatten mode
- **Material Slot**: Only for Paint Material mode (0-3)

### Actions

- **Flatten All**: Reset all heights to 0
- **Rebuild Mesh**: Force immediate mesh rebuild

All sculpt operations support full undo/redo.

---

## Platform-Specific Behavior

### Vulkan (Windows / Linux / Android)

- Full support with dedicated vertex/index buffer management
- Uses `VertexColor` vertex type with the `ForwardColor` shader pipeline
- Single draw call per Terrain3D node

### GameCube / Wii (GX)

- Full support via display list generation
- Uses the same vertex attribute layout as Voxel3D
- Consider smaller resolutions (64x64 or less) for memory constraints

### 3DS (C3D)

- Full support via double-buffered vertex/index data
- Same shader pipeline as static meshes
- Consider smaller resolutions for performance

### Material Compatibility

Terrain3D uses `MaterialLite` exclusively, ensuring compatibility across all platforms including Wii and 3DS.

---

## Serialization

Terrain data is embedded in scene files:

```
Format (ASSET_VERSION_TERRAIN3D = 23):
  - 2x int32: resolution (X, Z)
  - 3x float: world width, world depth, height scale
  - 1x float: snap grid size
  - uint32: heightmap element count
  - N floats: raw heightmap data
  - uint32: splatmap element count
  - N uint32: raw splatmap data (packed RGBA)
  - 4x AssetRef: material slot references
  - 1x AssetRef: heightmap texture reference
```

### Size Estimates

For a 128x128 terrain (16,384 vertices):
- Heightmap: ~64KB (16,384 floats)
- Splatmap: ~64KB (16,384 uint32s)
- Total scene overhead: ~128KB per terrain chunk

---

## Collision

Terrain3D generates a `btBvhTriangleMeshShape` from its mesh geometry:
- Built automatically when the mesh is rebuilt
- Supports physics raycasting and rigid body interaction
- Internal edge info is generated for smoother physics behavior
- Collision is updated every time the heightmap changes

---

## NavMesh Integration

Terrain3D nodes automatically contribute their triangles to the NavMesh system:
- `World::GatherNavTriangles()` includes Terrain3D triangles
- Terrain triangles are filtered by NavMesh3D bounds (same as StaticMesh3D)
- Wall culling is applied when configured in NavMesh3D volumes
- Call `BuildNavigationData()` after terrain changes to update pathfinding

---

## Terrain Snapping

Multiple terrain chunks can be placed edge-to-edge:
- Set `Snap Grid` property to match `World Width` (e.g., 128.0)
- Position is automatically snapped to multiples of the grid size
- All terrain chunks with the same resolution and world size will align their edges
- Height values at shared edges should match for seamless joins

---

## Configurable Properties

Properties exposed in the editor inspector:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| Resolution X | Integer | 128 | Grid vertices along X |
| Resolution Z | Integer | 128 | Grid vertices along Z |
| World Width | Float | 128.0 | World-space extent X |
| World Depth | Float | 128.0 | World-space extent Z |
| Height Scale | Float | 1.0 | Height value multiplier |
| Snap Grid | Float | 0.0 | Position snap grid (0=off) |
| Heightmap Texture | Texture | None | Source heightmap for import |
| Material Override | Material | None | Custom material (inherited) |

---

## Practical Guidance

### When to Use Terrain3D

**Good use cases:**
- Outdoor terrain and landscapes
- Height-based procedural generation
- Levels with large ground surfaces
- Multi-material terrain (grass, dirt, rock, snow)

**Consider alternatives when:**
- Cave interiors or overhangs (heightmaps can't represent overhangs)
- Very small detail areas (use StaticMesh3D instead)
- Voxel-based terrain (use Voxel3D)

### Performance Considerations

- **Resolution**: Keep reasonable (128x128 = 16K verts, 256x256 = 65K verts)
- **Rebuild frequency**: Batch height changes before the next tick
- **Draw calls**: Each Terrain3D node is one draw call
- **Memory**: Heightmap + splatmap + mesh vertices + GPU buffers
- **Collision**: Larger terrains have more expensive collision rebuilds

### Best Practices

1. **Batch modifications**: Set multiple heights before the next tick to avoid multiple rebuilds
2. **Use FlattenAll**: More efficient than individual SetHeight calls for initialization
3. **Import heightmaps**: Use SetHeightFromTexture for complex terrain shapes
4. **Snap for tiling**: Set Snap Grid = World Width when tiling multiple terrain chunks
5. **Resolution trade-off**: Lower resolution = faster rebuilds, higher = more detail

---

## Future Roadmap

Features planned for future phases:

1. **Custom terrain shader**: Per-pixel splatmap blending instead of vertex-color approach
2. **LOD / Geomipmapping**: Distance-based mesh simplification
3. **Chunked streaming**: Load/unload terrain chunks based on camera distance
4. **Edge stitching**: Automatic height averaging at shared edges between chunks
5. **Erosion tools**: Hydraulic and thermal erosion brushes
6. **Foliage painting**: Scatter instanced meshes based on splatmap weights

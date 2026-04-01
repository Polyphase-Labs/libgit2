#include "LuaBindings/Voxel3d_Lua.h"
#include "LuaBindings/Mesh3d_Lua.h"
#include "LuaBindings/Vector_Lua.h"
#include "LuaBindings/Asset_Lua.h"
#include "LuaBindings/Texture_Lua.h"
#include "LuaBindings/LuaUtils.h"

#if LUA_ENABLED

int Voxel3D_Lua::SetVoxel(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);
    int32_t x = CHECK_INTEGER(L, 2);
    int32_t y = CHECK_INTEGER(L, 3);
    int32_t z = CHECK_INTEGER(L, 4);
    int32_t value = CHECK_INTEGER(L, 5);

    voxel->SetVoxel(x, y, z, static_cast<VoxelType>(value));

    return 0;
}

int Voxel3D_Lua::GetVoxel(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);
    int32_t x = CHECK_INTEGER(L, 2);
    int32_t y = CHECK_INTEGER(L, 3);
    int32_t z = CHECK_INTEGER(L, 4);

    VoxelType value = voxel->GetVoxel(x, y, z);

    lua_pushinteger(L, static_cast<int32_t>(value));
    return 1;
}

int Voxel3D_Lua::Fill(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);
    int32_t value = CHECK_INTEGER(L, 2);

    voxel->Fill(static_cast<VoxelType>(value));

    return 0;
}

int Voxel3D_Lua::FillRegion(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);
    int32_t x0 = CHECK_INTEGER(L, 2);
    int32_t y0 = CHECK_INTEGER(L, 3);
    int32_t z0 = CHECK_INTEGER(L, 4);
    int32_t x1 = CHECK_INTEGER(L, 5);
    int32_t y1 = CHECK_INTEGER(L, 6);
    int32_t z1 = CHECK_INTEGER(L, 7);
    int32_t value = CHECK_INTEGER(L, 8);

    voxel->FillRegion(x0, y0, z0, x1, y1, z1, static_cast<VoxelType>(value));

    return 0;
}

int Voxel3D_Lua::MarkDirty(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);

    voxel->MarkDirty();

    return 0;
}

int Voxel3D_Lua::RebuildMesh(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);

    voxel->RebuildMesh();

    return 0;
}

int Voxel3D_Lua::IsDirty(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);

    bool dirty = voxel->IsDirty();

    lua_pushboolean(L, dirty);
    return 1;
}

int Voxel3D_Lua::GetDimensions(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);

    glm::ivec3 dims = voxel->GetDimensions();

    Vector_Lua::Create(L, glm::vec4(dims.x, dims.y, dims.z, 0.0f));
    return 1;
}

int Voxel3D_Lua::SetAtlasTexture(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);
    Texture* texture = nullptr;
    if (!lua_isnil(L, 2))
    {
        texture = CHECK_TEXTURE(L, 2);
    }

    uint32_t tilesX = 16;
    uint32_t tilesY = 16;
    if (lua_gettop(L) >= 3)
    {
        tilesX = static_cast<uint32_t>(lua_tointeger(L, 3));
    }
    if (lua_gettop(L) >= 4)
    {
        tilesY = static_cast<uint32_t>(lua_tointeger(L, 4));
    }

    voxel->SetAtlasTexture(texture, tilesX, tilesY);

    return 0;
}

int Voxel3D_Lua::GetAtlasTexture(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);

    Texture* texture = voxel->GetAtlasTexture();

    Asset_Lua::Create(L, texture);
    return 1;
}

int Voxel3D_Lua::SetAtlasEnabled(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);
    bool enabled = lua_toboolean(L, 2) != 0;

    voxel->SetAtlasEnabled(enabled);

    return 0;
}

int Voxel3D_Lua::IsAtlasEnabled(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);

    bool enabled = voxel->IsAtlasEnabled();

    lua_pushboolean(L, enabled);
    return 1;
}

int Voxel3D_Lua::SetMaterialTexture(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);
    int32_t materialId = static_cast<int32_t>(lua_tointeger(L, 2));

    int numArgs = lua_gettop(L);
    if (numArgs >= 5)
    {
        // 4-arg version: (id, top, bottom, side)
        uint16_t topTile = static_cast<uint16_t>(lua_tointeger(L, 3));
        uint16_t bottomTile = static_cast<uint16_t>(lua_tointeger(L, 4));
        uint16_t sideTile = static_cast<uint16_t>(lua_tointeger(L, 5));
        voxel->SetMaterialTexture(static_cast<VoxelType>(materialId), topTile, bottomTile, sideTile);
    }
    else
    {
        // 2-arg version: (id, allFacesTile)
        uint16_t tile = static_cast<uint16_t>(lua_tointeger(L, 3));
        voxel->SetMaterialTexture(static_cast<VoxelType>(materialId), tile);
    }

    return 0;
}

int Voxel3D_Lua::SetMaterialTint(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);
    int32_t materialId = static_cast<int32_t>(lua_tointeger(L, 2));
    glm::vec4 tint = CHECK_VECTOR(L, 3);

    voxel->SetMaterialTint(static_cast<VoxelType>(materialId), tint);

    return 0;
}

int Voxel3D_Lua::DisableMaterialTexture(lua_State* L)
{
    Voxel3D* voxel = CHECK_VOXEL_3D(L, 1);
    int32_t materialId = static_cast<int32_t>(lua_tointeger(L, 2));

    voxel->DisableMaterialTexture(static_cast<VoxelType>(materialId));

    return 0;
}

void Voxel3D_Lua::Bind()
{
    lua_State* L = GetLua();
    int mtIndex = CreateClassMetatable(
        VOXEL_3D_LUA_NAME,
        VOXEL_3D_LUA_FLAG,
        MESH_3D_LUA_NAME);

    Node_Lua::BindCommon(L, mtIndex);

    REGISTER_TABLE_FUNC(L, mtIndex, SetVoxel);
    REGISTER_TABLE_FUNC(L, mtIndex, GetVoxel);
    REGISTER_TABLE_FUNC(L, mtIndex, Fill);
    REGISTER_TABLE_FUNC(L, mtIndex, FillRegion);
    REGISTER_TABLE_FUNC(L, mtIndex, MarkDirty);
    REGISTER_TABLE_FUNC(L, mtIndex, RebuildMesh);
    REGISTER_TABLE_FUNC(L, mtIndex, IsDirty);
    REGISTER_TABLE_FUNC(L, mtIndex, GetDimensions);

    // Atlas texturing
    REGISTER_TABLE_FUNC(L, mtIndex, SetAtlasTexture);
    REGISTER_TABLE_FUNC(L, mtIndex, GetAtlasTexture);
    REGISTER_TABLE_FUNC(L, mtIndex, SetAtlasEnabled);
    REGISTER_TABLE_FUNC(L, mtIndex, IsAtlasEnabled);
    REGISTER_TABLE_FUNC(L, mtIndex, SetMaterialTexture);
    REGISTER_TABLE_FUNC(L, mtIndex, SetMaterialTint);
    REGISTER_TABLE_FUNC(L, mtIndex, DisableMaterialTexture);

    lua_pop(L, 1);
    OCT_ASSERT(lua_gettop(L) == 0);
}

#endif

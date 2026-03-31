#pragma once

#include "EngineTypes.h"
#include "Log.h"
#include "Engine.h"

#include "Nodes/3D/Voxel3d.h"

#include "LuaBindings/Node_Lua.h"
#include "LuaBindings/LuaUtils.h"

#if LUA_ENABLED

#define VOXEL_3D_LUA_NAME "Voxel3D"
#define VOXEL_3D_LUA_FLAG "cfVoxel3D"
#define CHECK_VOXEL_3D(L, arg) static_cast<Voxel3D*>(CheckNodeLuaType(L, arg, VOXEL_3D_LUA_NAME, VOXEL_3D_LUA_FLAG));

struct Voxel3D_Lua
{
    // Voxel access
    static int SetVoxel(lua_State* L);
    static int GetVoxel(lua_State* L);

    // Bulk operations
    static int Fill(lua_State* L);
    static int FillRegion(lua_State* L);

    // Mesh control
    static int MarkDirty(lua_State* L);
    static int RebuildMesh(lua_State* L);
    static int IsDirty(lua_State* L);

    // Dimensions
    static int GetDimensions(lua_State* L);

    static void Bind();
};

#endif

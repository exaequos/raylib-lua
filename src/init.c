#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "raylib.h"

#include <emscripten.h>

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define LuaPush_int(L, value)           lua_pushinteger(L, value)
#define LuaPush_float(L, value)         lua_pushnumber(L, value);
#define LuaPush_bool(L, value)          lua_pushboolean(L, value)
#define LuaPush_string(L, value)        lua_pushstring(L, value)
#define LuaPush_double(L, value)         lua_pushnumber(L, value);

#define LuaPush_Image(L, img)           LuaPushOpaqueTypeWithMetatable(L, img, Image)
#define LuaPush_Texture2D(L, tex)       LuaPushOpaqueTypeWithMetatable(L, tex, Texture2D)
#define LuaPush_RenderTexture2D(L, tex) LuaPushOpaqueTypeWithMetatable(L, tex, RenderTexture2D)
#define LuaPush_Font(L, sf)             LuaPushOpaqueTypeWithMetatable(L, sf, Font)
#define LuaPush_Mesh(L, vd)             LuaPushOpaqueType(L, vd)
#define LuaPush_Shader(L, s)            LuaPushOpaqueType(L, s)
#define LuaPush_Sound(L, snd)           LuaPushOpaqueType(L, snd)
#define LuaPush_Wave(L, wav)            LuaPushOpaqueType(L, wav)
#define LuaPush_Music(L, mus)           LuaPushOpaqueType(L, mus)
#define LuaPush_AudioStream(L, aud)     LuaPushOpaqueType(L, aud)
#define LuaPush_PhysicsBody(L, body)    LuaPushOpaqueType(L, body)

#define LuaGetArgument_ptr              (void *)luaL_checkinteger
#define LuaGetArgument_int              (int)luaL_checkinteger
#define LuaGetArgument_unsigned         (unsigned)luaL_checkinteger
#define LuaGetArgument_char             (char)luaL_checkinteger
#define LuaGetArgument_float            (float)luaL_checknumber
#define LuaGetArgument_double           luaL_checknumber
#define LuaGetArgument_string           luaL_checkstring

#define LuaGetArgument_Image(L, img)        *(Image *)LuaGetArgumentOpaqueTypeWithMetatable(L, img, "Image")
#define LuaGetArgument_Texture2D(L, tex)    *(Texture2D *)LuaGetArgumentOpaqueTypeWithMetatable(L, tex, "Texture2D")
#define LuaGetArgument_RenderTexture2D(L, rtex) *(RenderTexture2D *)LuaGetArgumentOpaqueTypeWithMetatable(L, rtex, "RenderTexture2D")
#define LuaGetArgument_Font(L, sf)          *(Font *)LuaGetArgumentOpaqueTypeWithMetatable(L, sf, "Font")
#define LuaGetArgument_Mesh(L, vd)          *(Mesh *)LuaGetArgumentOpaqueType(L, vd)
#define LuaGetArgument_Shader(L, s)         *(Shader *)LuaGetArgumentOpaqueType(L, s)
#define LuaGetArgument_Sound(L, snd)        *(Sound *)LuaGetArgumentOpaqueType(L, snd)
#define LuaGetArgument_Wave(L, wav)         *(Wave *)LuaGetArgumentOpaqueType(L, wav)
#define LuaGetArgument_Music(L, mus)        *(Music *)LuaGetArgumentOpaqueType(L, mus)
#define LuaGetArgument_AudioStream(L, aud)  *(AudioStream *)LuaGetArgumentOpaqueType(L, aud)
#define LuaGetArgument_PhysicsBody(L, body) *(PhysicsBody *)LuaGetArgumentOpaqueType(L, body)

#define LuaPushOpaqueType(L, str)                    LuaPushOpaque(L, &str, sizeof(str))
#define LuaPushOpaqueTypeWithMetatable(L, str, meta) LuaPushOpaqueWithMetatable(L, &str, sizeof(str), #meta)

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//static lua_State* mainLuaState = 0;
static lua_State* L = 0;

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
static void LuaPush_Color(lua_State* L, Color color);
static void LuaPush_Vector2(lua_State* L, Vector2 vec);
static void LuaPush_Vector3(lua_State* L, Vector3 vec);
static void LuaPush_Vector4(lua_State* L, Vector4 vec);
static void LuaPush_Quaternion(lua_State* L, Quaternion vec);
static void LuaPush_Matrix(lua_State* L, Matrix *matrix);
static void LuaPush_Rectangle(lua_State* L, Rectangle rect);
static void LuaPush_Camera(lua_State* L, Camera cam);
static void LuaPush_Camera2D(lua_State* L, Camera2D cam);
static void LuaPush_Model(lua_State* L, Model mdl);
static void LuaPush_Ray(lua_State* L, Ray ray);

static Color LuaGetArgument_Color(lua_State* L, int index);
static Vector2 LuaGetArgument_Vector2(lua_State* L, int index);
static Vector3 LuaGetArgument_Vector3(lua_State* L, int index);
static Vector4 LuaGetArgument_Vector4(lua_State* L, int index);
static Quaternion LuaGetArgument_Quaternion(lua_State* L, int index);
static Matrix LuaGetArgument_Matrix(lua_State* L, int index);
static Rectangle LuaGetArgument_Rectangle(lua_State* L, int index);
static Camera LuaGetArgument_Camera(lua_State* L, int index);
static Camera2D LuaGetArgument_Camera2D(lua_State* L, int index);
static Model LuaGetArgument_Model(lua_State* L, int index);
static Ray LuaGetArgument_Ray(lua_State* L, int index);

//----------------------------------------------------------------------------------
// rlua Helper Functions
//----------------------------------------------------------------------------------
static void LuaStartEnum(void)
{
    lua_newtable(L);
}

static void LuaSetEnum(const char *name, int value)
{
    LuaPush_int(L, value);
    lua_setfield(L, -2, name);
}

static void LuaSetEnumColor(const char *name, Color color)
{
    LuaPush_Color(L, color);
    lua_setfield(L, -2, name);
}

static void LuaEndEnum(const char *name)
{
    lua_setglobal(L, name);
}

static void LuaPushOpaque(lua_State* L, void *ptr, size_t size)
{
    void *ud = lua_newuserdata(L, size);
    memcpy(ud, ptr, size);
}

static void LuaPushOpaqueWithMetatable(lua_State* L, void *ptr, size_t size, const char *metatable_name)
{
    void *ud = lua_newuserdata(L, size);
    memcpy(ud, ptr, size);
    luaL_setmetatable(L, metatable_name);
}

static void* LuaGetArgumentOpaqueType(lua_State* L, int index)
{
    return lua_touserdata(L, index);
}

static void* LuaGetArgumentOpaqueTypeWithMetatable(lua_State* L, int index, const char *metatable_name)
{
    return luaL_checkudata(L, index, metatable_name);
}

//----------------------------------------------------------------------------------
// LuaIndex* functions
//----------------------------------------------------------------------------------
static int LuaIndexImage(lua_State* L)
{
    Image img = LuaGetArgument_Image(L, 1);
    const char *key = luaL_checkstring(L, 2);
    
    if (!strcmp(key, "width")) LuaPush_int(L, img.width);
    else if (!strcmp(key, "height")) LuaPush_int(L, img.height);
    else if (!strcmp(key, "mipmaps")) LuaPush_int(L, img.mipmaps);
    else if (!strcmp(key, "format")) LuaPush_int(L, img.format);
    else return 0;
    
    return 1;
}

static int LuaIndexTexture2D(lua_State* L)
{
    Texture2D img = LuaGetArgument_Texture2D(L, 1);
    const char *key = luaL_checkstring(L, 2);
    
    if (!strcmp(key, "width")) LuaPush_int(L, img.width);
    else if (!strcmp(key, "height")) LuaPush_int(L, img.height);
    else if (!strcmp(key, "mipmaps")) LuaPush_int(L, img.mipmaps);
    else if (!strcmp(key, "format")) LuaPush_int(L, img.format);
    else if (!strcmp(key, "id")) LuaPush_int(L, img.id);
    else return 0;

    return 1;
}

static int LuaIndexRenderTexture2D(lua_State* L)
{
    RenderTexture2D img = LuaGetArgument_RenderTexture2D(L, 1);
    const char *key = luaL_checkstring(L, 2);
    
    if (!strcmp(key, "texture")) LuaPush_Texture2D(L, img.texture);
    else if (!strcmp(key, "depth")) LuaPush_Texture2D(L, img.depth);
    else return 0;
    
    return 1;
}

static int LuaIndexFont(lua_State* L)
{
    Font img = LuaGetArgument_Font(L, 1);
    const char *key = luaL_checkstring(L, 2);
    
    if (!strcmp(key, "baseSize")) LuaPush_int(L, img.baseSize);
    else if (!strcmp(key, "texture")) LuaPush_Texture2D(L, img.texture);
    else if (!strcmp(key, "glyphCount")) LuaPush_int(L, img.glyphCount);
    else return 0;
    
    return 1;
}

static void LuaBuildOpaqueMetatables(void)
{
    luaL_newmetatable(L, "Image");
    lua_pushcfunction(L, &LuaIndexImage);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newmetatable(L, "Texture2D");
    lua_pushcfunction(L, &LuaIndexTexture2D);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newmetatable(L, "RenderTexture2D");
    lua_pushcfunction(L, &LuaIndexRenderTexture2D);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newmetatable(L, "Font");
    lua_pushcfunction(L, &LuaIndexFont);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

//----------------------------------------------------------------------------------
// LuaGetArgument functions
//----------------------------------------------------------------------------------

// Vector2 type
static Vector2 LuaGetArgument_Vector2(lua_State *L, int index)
{
    Vector2 result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "x") == LUA_TNUMBER, index, "Expected Vector2.x");
    result.x = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "y") == LUA_TNUMBER, index, "Expected Vector2.y");
    result.y = LuaGetArgument_float(L, -1);
    lua_pop(L, 2);

    //emscripten_log(EM_LOG_CONSOLE, "Vector2: %f %f", result.x, result.y);
    return result;
}

// Vector3 type
static Vector3 LuaGetArgument_Vector3(lua_State *L, int index)
{
    Vector3 result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "x") == LUA_TNUMBER, index, "Expected Vector3.x");
    result.x = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "y") == LUA_TNUMBER, index, "Expected Vector3.y");
    result.y = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "z") == LUA_TNUMBER, index, "Expected Vector3.z");
    result.z = LuaGetArgument_float(L, -1);
    lua_pop(L, 3);
    return result;
}

// Vector4 type
static Vector4 LuaGetArgument_Vector4(lua_State *L, int index)
{
    Vector4 result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "x") == LUA_TNUMBER, index, "Expected Vector4.x");
    result.x = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "y") == LUA_TNUMBER, index, "Expected Vector4.y");
    result.y = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "z") == LUA_TNUMBER, index, "Expected Vector4.z");
    result.z = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "w") == LUA_TNUMBER, index, "Expected Vector4.w");
    result.w = LuaGetArgument_float(L, -1);
    lua_pop(L, 4);
    return result;
}

static Quaternion LuaGetArgument_Quaternion(lua_State* L, int index)
{
    Quaternion result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "x") == LUA_TNUMBER, index, "Expected Quaternion.x");
    result.x = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "y") == LUA_TNUMBER, index, "Expected Quaternion.y");
    result.y = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "z") == LUA_TNUMBER, index, "Expected Quaternion.z");
    result.z = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "w") == LUA_TNUMBER, index, "Expected Quaternion.w");
    result.w = LuaGetArgument_float(L, -1);
    lua_pop(L, 4);
    return result;
}

// Matrix type
static Matrix LuaGetArgument_Matrix(lua_State* L, int index)
{
    Matrix result = { 0 };
    float* ptr = &result.m0;
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values

    for (int i = 0; i < 16; i++)
    {
        lua_geti(L, index, i+1);
        ptr[i] = luaL_checknumber(L, -1);
    }
    lua_pop(L, 16);
    return result;
}

// Color type, RGBA (32bit)
static Color LuaGetArgument_Color(lua_State *L, int index)
{
    Color result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    //emscripten_log(EM_LOG_CONSOLE, "lua_absindex -> %d", index);
    luaL_argcheck(L, lua_getfield(L, index, "r") == LUA_TNUMBER, index, "Expected Color.r");
    result.r = LuaGetArgument_unsigned(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "g") == LUA_TNUMBER, index, "Expected Color.g");
    result.g = LuaGetArgument_unsigned(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "b") == LUA_TNUMBER, index, "Expected Color.b");
    result.b = LuaGetArgument_unsigned(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "a") == LUA_TNUMBER, index, "Expected Color.a");
    result.a = LuaGetArgument_unsigned(L, -1);
    //emscripten_log(EM_LOG_CONSOLE, "LuaGetArgument_Color -> %d %d %d", result.r, result.g, result.b);
    lua_pop(L, 4);
    return result;
}

// Rectangle type
static Rectangle LuaGetArgument_Rectangle(lua_State *L, int index)
{
    Rectangle result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "x") == LUA_TNUMBER, index, "Expected Rectangle.x");
    result.x = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "y") == LUA_TNUMBER, index, "Expected Rectangle.y");
    result.y = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "width") == LUA_TNUMBER, index, "Expected Rectangle.width");
    result.width = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "height") == LUA_TNUMBER, index, "Expected Rectangle.height");
    result.height = LuaGetArgument_float(L, -1);
    lua_pop(L, 4);
    return result;
}

// Image type -> Opaque

// Texture2D type -> Opaque

// RenderTexture2D type -> Opaque

// Font character info type
static GlyphInfo LuaGetArgument_GlyphInfo(lua_State *L, int index)
{
    GlyphInfo result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "value") == LUA_TNUMBER, index, "Expected CharInfo.value");
    result.value = LuaGetArgument_int(L, -1);
    //luaL_argcheck(L, lua_getfield(L, index, "rec") == LUA_TNUMBER, index, "Expected CharInfo.rec");
    //result.rec = LuaGetArgument_Rectangle(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "offsetX") == LUA_TNUMBER, index, "Expected CharInfo.offsetX");
    result.offsetX = LuaGetArgument_int(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "offsetY") == LUA_TNUMBER, index, "Expected CharInfo.offsetY");
    result.offsetY = LuaGetArgument_int(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "advanceX") == LUA_TNUMBER, index, "Expected CharInfo.advanceX");
    result.advanceX = LuaGetArgument_int(L, -1);
    //luaL_argcheck(L, lua_getfield(L, index, "char *data") == LUA_TNUMBER, index, "Expected CharInfo.char *data");
    //result.char *data = LuaGetArgument_unsigned(L, -1);
    lua_pop(L, 6);
    return result;
}

// Font type -> Opaque

// Camera type, defines a camera position/orientation in 3d space
static Camera LuaGetArgument_Camera(lua_State *L, int index)
{
    Camera result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "position") == LUA_TTABLE, index, "Expected Camera.position");
    result.position = LuaGetArgument_Vector3(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "target") == LUA_TTABLE, index, "Expected Camera.target");
    result.target = LuaGetArgument_Vector3(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "up") == LUA_TTABLE, index, "Expected Camera.up");
    result.up = LuaGetArgument_Vector3(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "fovy") == LUA_TNUMBER, index, "Expected Camera.fovy");
    result.fovy = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "projection") == LUA_TNUMBER, index, "Expected Camera.projection");
    result.projection = LuaGetArgument_int(L, -1);
    lua_pop(L, 5);
    return result;
}

// Camera2D type, defines a 2d camera
static Camera2D LuaGetArgument_Camera2D(lua_State *L, int index)
{
    Camera2D result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "offset") == LUA_TTABLE, index, "Expected Camera2D.offset");
    result.offset = LuaGetArgument_Vector2(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "target") == LUA_TTABLE, index, "Expected Camera2D.target");
    result.target = LuaGetArgument_Vector2(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "rotation") == LUA_TNUMBER, index, "Expected Camera2D.rotation");
    result.rotation = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "zoom") == LUA_TNUMBER, index, "Expected Camera2D.zoom");
    result.zoom = LuaGetArgument_float(L, -1);
    lua_pop(L, 4);
    return result;
}

// Bounding box type
static BoundingBox LuaGetArgument_BoundingBox(lua_State *L, int index)
{
    BoundingBox result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "min") == LUA_TTABLE, index, "Expected BoundingBox.min");
    result.min = LuaGetArgument_Vector3(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "max") == LUA_TTABLE, index, "Expected BoundingBox.max");
    result.max = LuaGetArgument_Vector3(L, -1);
    lua_pop(L, 2);
    return result;
}

// Mesh type -> Opaque

// Shader type -> Opaque

// Material texture map
static MaterialMap LuaGetArgument_MaterialMap(lua_State *L, int index)
{
    MaterialMap result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "texture") == LUA_TNUMBER, index, "Expected MaterialMap.texture");
    result.texture = LuaGetArgument_Texture2D(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "color") == LUA_TTABLE, index, "Expected MaterialMap.color");
    result.color = LuaGetArgument_Color(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "value") == LUA_TNUMBER, index, "Expected MaterialMap.value");
    result.value = LuaGetArgument_float(L, -1);
    lua_pop(L, 3);
    return result;
}

// Material type -> REVIEW
/*
static Material LuaGetArgument_Material(lua_State *L, int index)
{
    Material result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "shader") == LUA_TNUMBER, index, "Expected Material.shader");
    result.shader = LuaGetArgument_Shader(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "maps[MAX_MATERIAL_MAPS]") == LUA_TNUMBER, index, "Expected Material.maps[MAX_MATERIAL_MAPS]");
    result.maps[MAX_MATERIAL_MAPS] = LuaGetArgument_MaterialMap(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "*params") == LUA_TNUMBER, index, "Expected Material.*params");
    result.*params = LuaGetArgument_float(L, -1);
    lua_pop(L, 3);
    return result;
}

static Material LuaGetArgument_Material(lua_State* L, int index)
{
    Material result;
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "shader") == LUA_TUSERDATA, index, "Expected Material");
    result.shader = LuaGetArgument_Shader(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "texDiffuse") == LUA_TUSERDATA, index, "Expected Material");
    result.texDiffuse = LuaGetArgument_Texture2D(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "texNormal") == LUA_TUSERDATA, index, "Expected Material");
    result.texNormal = LuaGetArgument_Texture2D(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "texSpecular") == LUA_TUSERDATA, index, "Expected Material");
    result.texSpecular = LuaGetArgument_Texture2D(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "colDiffuse") == LUA_TTABLE, index, "Expected Material");
    result.colDiffuse = LuaGetArgument_Color(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "colAmbient") == LUA_TTABLE, index, "Expected Material");
    result.colAmbient = LuaGetArgument_Color(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "colSpecular") == LUA_TTABLE, index, "Expected Material");
    result.colSpecular = LuaGetArgument_Color(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "glossiness") == LUA_TNUMBER, index, "Expected Material");
    result.glossiness = LuaGetArgument_float(L, -1);
    lua_pop(L, 8);
    return result;
}
*/

#if 0
// Model type
static Model LuaGetArgument_Model(lua_State *L, int index)
{
    Model result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "mesh") == LUA_TNUMBER, index, "Expected Model.mesh");
    result.mesh = LuaGetArgument_Mesh(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "transform") == LUA_TNUMBER, index, "Expected Model.transform");
    result.transform = LuaGetArgument_Matrix(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "material") == LUA_TNUMBER, index, "Expected Model.material");
    result.material = LuaGetArgument_Material(L, -1);
    lua_pop(L, 3);
    return result;
}
#endif

// Ray type (useful for raycast)
static Ray LuaGetArgument_Ray(lua_State *L, int index)
{
    Ray result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "position") == LUA_TTABLE, index, "Expected Ray.position");
    result.position = LuaGetArgument_Vector3(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "direction") == LUA_TTABLE, index, "Expected Ray.direction");
    result.direction = LuaGetArgument_Vector3(L, -1);
    lua_pop(L, 2);
    return result;
}

// Wave type -> Opaque

// Sound type -> Opaque

// MusicData type -> Opaque

#if 0

// Head-Mounted-Display device parameters
static VrDeviceInfo LuaGetArgument_VrDeviceInfo(lua_State *L, int index)
{
    VrDeviceInfo result = { 0 };
    index = lua_absindex(L, index); // Makes sure we use absolute indices because we push multiple values
    luaL_argcheck(L, lua_getfield(L, index, "hResolution") == LUA_TNUMBER, index, "Expected VrDeviceInfo.hResolution");
    result.hResolution = LuaGetArgument_int(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "vResolution") == LUA_TNUMBER, index, "Expected VrDeviceInfo.vResolution");
    result.vResolution = LuaGetArgument_int(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "hScreenSize") == LUA_TNUMBER, index, "Expected VrDeviceInfo.hScreenSize");
    result.hScreenSize = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "vScreenSize") == LUA_TNUMBER, index, "Expected VrDeviceInfo.vScreenSize");
    result.vScreenSize = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "vScreenCenter") == LUA_TNUMBER, index, "Expected VrDeviceInfo.vScreenCenter");
    result.vScreenCenter = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "eyeToScreenDistance") == LUA_TNUMBER, index, "Expected VrDeviceInfo.eyeToScreenDistance");
    result.eyeToScreenDistance = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "lensSeparationDistance") == LUA_TNUMBER, index, "Expected VrDeviceInfo.lensSeparationDistance");
    result.lensSeparationDistance = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "interpupillaryDistance") == LUA_TNUMBER, index, "Expected VrDeviceInfo.interpupillaryDistance");
    result.interpupillaryDistance = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "lensDistortionValues[4]") == LUA_TNUMBER, index, "Expected VrDeviceInfo.lensDistortionValues[4]");
    result.lensDistortionValues[4] = LuaGetArgument_float(L, -1);
    luaL_argcheck(L, lua_getfield(L, index, "chromaAbCorrection[4]") == LUA_TNUMBER, index, "Expected VrDeviceInfo.chromaAbCorrection[4]");
    result.chromaAbCorrection[4] = LuaGetArgument_float(L, -1);
    lua_pop(L, 10);
    return result;
}
#endif

//----------------------------------------------------------------------------------
// LuaPush functions
//----------------------------------------------------------------------------------
static void LuaPush_Color(lua_State* L, Color color)
{
    lua_createtable(L, 0, 4);
    LuaPush_int(L, color.r);
    lua_setfield(L, -2, "r");
    LuaPush_int(L, color.g);
    lua_setfield(L, -2, "g");
    LuaPush_int(L, color.b);
    lua_setfield(L, -2, "b");
    LuaPush_int(L, color.a);
    lua_setfield(L, -2, "a");
}

static void LuaPush_Vector2(lua_State* L, Vector2 vec)
{
    lua_createtable(L, 0, 2);
    LuaPush_float(L, vec.x);
    lua_setfield(L, -2, "x");
    LuaPush_float(L, vec.y);
    lua_setfield(L, -2, "y");
}

static void LuaPush_Vector3(lua_State* L, Vector3 vec)
{
    lua_createtable(L, 0, 3);
    LuaPush_float(L, vec.x);
    lua_setfield(L, -2, "x");
    LuaPush_float(L, vec.y);
    lua_setfield(L, -2, "y");
    LuaPush_float(L, vec.z);
    lua_setfield(L, -2, "z");
}

static void LuaPush_Vector4(lua_State* L, Vector4 vec)
{
    lua_createtable(L, 0, 4);
    LuaPush_float(L, vec.x);
    lua_setfield(L, -2, "x");
    LuaPush_float(L, vec.y);
    lua_setfield(L, -2, "y");
    LuaPush_float(L, vec.z);
    lua_setfield(L, -2, "z");
    LuaPush_float(L, vec.w);
    lua_setfield(L, -2, "w");
}

static void LuaPush_Quaternion(lua_State* L, Quaternion vec)
{
    lua_createtable(L, 0, 4);
    LuaPush_float(L, vec.x);
    lua_setfield(L, -2, "x");
    LuaPush_float(L, vec.y);
    lua_setfield(L, -2, "y");
    LuaPush_float(L, vec.z);
    lua_setfield(L, -2, "z");
    LuaPush_float(L, vec.w);
    lua_setfield(L, -2, "w");
}

static void LuaPush_Matrix(lua_State* L, Matrix *matrix)
{
    int i;
    lua_createtable(L, 16, 0);
    float* num = (&matrix->m0);
    for (i = 0; i < 16; i++)
    {
        LuaPush_float(L, num[i]);
        lua_rawseti(L, -2, i + 1);
    }
}

static void LuaPush_Rectangle(lua_State* L, Rectangle rect)
{
    lua_createtable(L, 0, 4);
    LuaPush_int(L, rect.x);
    lua_setfield(L, -2, "x");
    LuaPush_int(L, rect.y);
    lua_setfield(L, -2, "y");
    LuaPush_int(L, rect.width);
    lua_setfield(L, -2, "width");
    LuaPush_int(L, rect.height);
    lua_setfield(L, -2, "height");
}

static void LuaPush_Ray(lua_State* L, Ray ray)
{
    lua_createtable(L, 0, 2);
    LuaPush_Vector3(L, ray.position);
    lua_setfield(L, -2, "position");
    LuaPush_Vector3(L, ray.direction);
    lua_setfield(L, -2, "direction");
}

static void LuaPush_BoundingBox(lua_State* L, BoundingBox bb)
{
    lua_createtable(L, 0, 2);
    LuaPush_Vector3(L, bb.min);
    lua_setfield(L, -2, "min");
    LuaPush_Vector3(L, bb.max);
    lua_setfield(L, -2, "max");
}

static void LuaPush_Camera(lua_State* L, Camera cam)
{
    lua_createtable(L, 0, 4);
    LuaPush_Vector3(L, cam.position);
    lua_setfield(L, -2, "position");
    LuaPush_Vector3(L, cam.target);
    lua_setfield(L, -2, "target");
    LuaPush_Vector3(L, cam.up);
    lua_setfield(L, -2, "up");
    LuaPush_float(L, cam.fovy);
    lua_setfield(L, -2, "fovy");
    LuaPush_int(L, cam.projection);
    lua_setfield(L, -2, "projection");
}

static void LuaPush_Camera2D(lua_State* L, Camera2D cam)
{
    lua_createtable(L, 0, 4);
    LuaPush_Vector2(L, cam.offset);
    lua_setfield(L, -2, "offset");
    LuaPush_Vector2(L, cam.target);
    lua_setfield(L, -2, "target");
    LuaPush_float(L, cam.rotation);
    lua_setfield(L, -2, "rotation");
    LuaPush_float(L, cam.zoom);
    lua_setfield(L, -2, "zoom");
}

// REVIEW!!!
/*
static void LuaPush_Material(lua_State* L, Material mat)
{
    lua_createtable(L, 0, 8);
    LuaPush_Shader(L, mat.shader);
    lua_setfield(L, -2, "shader");
    LuaPush_Texture2D(L, mat.texDiffuse);
    lua_setfield(L, -2, "texDiffuse");
    LuaPush_Texture2D(L, mat.texNormal);
    lua_setfield(L, -2, "texNormal");
    LuaPush_Texture2D(L, mat.texSpecular);
    lua_setfield(L, -2, "texSpecular");
    LuaPush_Color(L, mat.colDiffuse);
    lua_setfield(L, -2, "colDiffuse");
    LuaPush_Color(L, mat.colAmbient);
    lua_setfield(L, -2, "colAmbient");
    LuaPush_Color(L, mat.colSpecular);
    lua_setfield(L, -2, "colSpecular");
    LuaPush_float(L, mat.glossiness);
    lua_setfield(L, -2, "glossiness");
}
*/

#if 0
static void LuaPush_Model(lua_State* L, Model mdl)
{
    lua_createtable(L, 0, 3);
    LuaPush_Mesh(L, mdl.mesh);
    lua_setfield(L, -2, "mesh");
    LuaPush_Matrix(L, &mdl.transform);
    lua_setfield(L, -2, "transform");
    LuaPush_Material(L, mdl.material);
    lua_setfield(L, -2, "material");
}
#endif

//----------------------------------------------------------------------------------
// raylib Lua Structure constructors
//----------------------------------------------------------------------------------
static int lua_Color(lua_State* L)
{
    LuaPush_Color(L, (Color) { (unsigned char)luaL_checkinteger(L, 1), (unsigned char)luaL_checkinteger(L, 2), (unsigned char)luaL_checkinteger(L, 3), (unsigned char)luaL_checkinteger(L, 4) });
    return 1;
}

static int lua_Vector2(lua_State* L)
{
    LuaPush_Vector2(L, (Vector2) { (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2) });
    return 1;
}

static int lua_Vector3(lua_State* L)
{
    LuaPush_Vector3(L, (Vector3) { (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3) });
    return 1;
}

static int lua_Vector4(lua_State* L)
{
    LuaPush_Vector4(L, (Vector4) { (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4) });
    return 1;
}

static int lua_Quaternion(lua_State* L)
{
    LuaPush_Quaternion(L, (Quaternion) { (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4) });
    return 1;
}

static int lua_Rectangle(lua_State* L)
{
    LuaPush_Rectangle(L, (Rectangle) { (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4) });
    return 1;
}

static int lua_Ray(lua_State* L)
{
    Vector3 pos = LuaGetArgument_Vector3(L, 1);
    Vector3 dir = LuaGetArgument_Vector3(L, 2);
    LuaPush_Ray(L, (Ray) { { pos.x, pos.y, pos.z }, { dir.x, dir.y, dir.z } });
    return 1;
}

static int lua_BoundingBox(lua_State* L)
{
    Vector3 min = LuaGetArgument_Vector3(L, 1);
    Vector3 max = LuaGetArgument_Vector3(L, 2);
    LuaPush_BoundingBox(L, (BoundingBox) { { min.x, min.y, min.z }, { max.x, max.y, max.z } });
    return 1;
}

static int lua_Camera(lua_State* L)
{
    Vector3 pos = LuaGetArgument_Vector3(L, 1);
    Vector3 tar = LuaGetArgument_Vector3(L, 2);
    Vector3 up = LuaGetArgument_Vector3(L, 3);
    float fovy = LuaGetArgument_float(L, 4);
    int projection = LuaGetArgument_int(L, 5);
    LuaPush_Camera(L, (Camera) { { pos.x, pos.y, pos.z }, { tar.x, tar.y, tar.z }, { up.x, up.y, up.z }, fovy, projection });
    return 1;
}

static int lua_Camera2D(lua_State* L)
{
    Vector2 off = LuaGetArgument_Vector2(L, 1);
    Vector2 tar = LuaGetArgument_Vector2(L, 2);
    float rot = LuaGetArgument_float(L, 3);
    float zoom = LuaGetArgument_float(L, 4);
    LuaPush_Camera2D(L, (Camera2D) { { off.x, off.y }, { tar.x, tar.y }, rot, zoom });
    return 1;
}

#define GET_TABLE(type, name, index) \
    type* name = 0; \
    size_t name##_size = 0; \
    { \
        size_t sz = 0; \
        luaL_checktype(L, index, LUA_TTABLE); \
        lua_pushnil(L); \
        while (lua_next(L, index)) { \
            LuaGetArgument_##type(L, -1); \
            sz++; \
            lua_pop(L, 1); \
        } \
        name = calloc(sz, sizeof(type)); \
        sz = 0; \
        lua_pushnil(L); \
        while (lua_next(L, index)) { \
            name[sz] = LuaGetArgument_##type(L, -1); \
            sz++; \
            lua_pop(L, 1); \
        } \
        name##_size = sz; \
    }

/*************************************************************************************
*
*  raylib Lua Functions Bindings
*
**************************************************************************************/

//------------------------------------------------------------------------------------
// raylib [core] module functions - Window and Graphics Device
//------------------------------------------------------------------------------------

// Initialize window and OpenGL context
int lua_InitWindow(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    const char *title = LuaGetArgument_string(L, 3);
    InitWindow(width, height, title);
    return 0;
}

// Close window and unload OpenGL context
int lua_CloseWindow(lua_State *L)
{
    CloseWindow();
    return 0;
}

// Check if window has been initialized successfully
int lua_IsWindowReady(lua_State *L)
{
    bool result = IsWindowReady();
    LuaPush_bool(L, result);
    return 1;
}

// Check if KEY_ESCAPE pressed or Close icon pressed
int lua_WindowShouldClose(lua_State *L)
{
    bool result = WindowShouldClose();
    LuaPush_bool(L, result);
    return 1;
}

// Check if window has been minimized (or lost focus)
int lua_IsWindowMinimized(lua_State *L)
{
    bool result = IsWindowMinimized();
    LuaPush_bool(L, result);
    return 1;
}

// Toggle fullscreen mode (only PLATFORM_DESKTOP)
int lua_ToggleFullscreen(lua_State *L)
{
    ToggleFullscreen();
    return 0;
}

// Set icon for window (only PLATFORM_DESKTOP)
int lua_SetWindowIcon(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    SetWindowIcon(image);
    return 0;
}

// Set title for window (only PLATFORM_DESKTOP)
int lua_SetWindowTitle(lua_State *L)
{
    const char *title = LuaGetArgument_string(L, 1);
    SetWindowTitle(title);
    return 0;
}

// Set window position on screen (only PLATFORM_DESKTOP)
int lua_SetWindowPosition(lua_State *L)
{
    int x = LuaGetArgument_int(L, 1);
    int y = LuaGetArgument_int(L, 2);
    SetWindowPosition(x, y);
    return 0;
}

// Set monitor for the current window (fullscreen mode)
int lua_SetWindowMonitor(lua_State *L)
{
    int monitor = LuaGetArgument_int(L, 1);
    SetWindowMonitor(monitor);
    return 0;
}

// Set window minimum dimensions (for FLAG_WINDOW_RESIZABLE)
int lua_SetWindowMinSize(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    SetWindowMinSize(width, height);
    return 0;
}

// Set window dimensions
int lua_SetWindowSize(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    SetWindowSize(width, height);
    return 0;
}

// Get current screen width
int lua_GetScreenWidth(lua_State *L)
{
    int result = GetScreenWidth();
    LuaPush_int(L, result);
    return 1;
}

// Get current screen height
int lua_GetScreenHeight(lua_State *L)
{
    int result = GetScreenHeight();
    LuaPush_int(L, result);
    return 1;
}

// Cursor-related functions
// Shows cursor
int lua_ShowCursor(lua_State *L)
{
    ShowCursor();
    return 0;
}

// Hides cursor
int lua_HideCursor(lua_State *L)
{
    HideCursor();
    return 0;
}

// Check if cursor is not visible
int lua_IsCursorHidden(lua_State *L)
{
    bool result = IsCursorHidden();
    LuaPush_bool(L, result);
    return 1;
}

// Enables cursor (unlock cursor)
int lua_EnableCursor(lua_State *L)
{
    EnableCursor();
    return 0;
}

// Disables cursor (lock cursor)
int lua_DisableCursor(lua_State *L)
{
    DisableCursor();
    return 0;
}

// Drawing-related functions
// Set background color (framebuffer clear color)
int lua_ClearBackground(lua_State *L)
{
    Color color = LuaGetArgument_Color(L, 1);
    ClearBackground(color);
    return 0;
}

// Setup canvas (framebuffer) to start drawing
int lua_BeginDrawing(lua_State *L)
{
    BeginDrawing();
    return 0;
}

// End canvas drawing and swap buffers (double buffering)
int lua_EndDrawing(lua_State *L)
{
    EndDrawing();
    return 0;
}

// Initialize 2D mode with custom camera (2D)
int lua_BeginMode2D(lua_State *L)
{
    Camera2D camera = LuaGetArgument_Camera2D(L, 1);
    BeginMode2D(camera);
    return 0;
}

// Ends 2D mode with custom camera
int lua_EndMode2D(lua_State *L)
{
    EndMode2D();
    return 0;
}

// Initializes 3D mode with custom camera (3D)
int lua_BeginMode3D(lua_State *L)
{
    Camera3D camera = LuaGetArgument_Camera(L, 1);
    BeginMode3D(camera);
    return 0;
}

// Ends 3D mode and returns to default 2D orthographic mode
int lua_EndMode3D(lua_State *L)
{
    EndMode3D();
    return 0;
}

// Initializes render texture for drawing
int lua_BeginTextureMode(lua_State *L)
{
    RenderTexture2D target = LuaGetArgument_RenderTexture2D(L, 1);
    BeginTextureMode(target);
    return 0;
}

// Ends drawing to render texture
int lua_EndTextureMode(lua_State *L)
{
    EndTextureMode();
    return 0;
}

// Screen-space-related functions
// Returns a ray trace from mouse position
int lua_GetMouseRay(lua_State *L)
{
    Vector2 mousePosition = LuaGetArgument_Vector2(L, 1);
    Camera camera = LuaGetArgument_Camera(L, 2);
    Ray result = GetMouseRay(mousePosition, camera);
    LuaPush_Ray(L, result);
    return 1;
}

// Returns the screen space position for a 3d world space position
int lua_GetWorldToScreen(lua_State *L)
{
    Vector3 position = LuaGetArgument_Vector3(L, 1);
    Camera camera = LuaGetArgument_Camera(L, 2);
    Vector2 result = GetWorldToScreen(position, camera);
    LuaPush_Vector2(L, result);
    return 1;
}

// Returns camera transform matrix (view matrix)
int lua_GetCameraMatrix(lua_State *L)
{
    Camera camera = LuaGetArgument_Camera(L, 1);
    Matrix result = GetCameraMatrix(camera);
    LuaPush_Matrix(L, &result);
    return 1;
}

// Timming-related functions
// Set target FPS (maximum)
int lua_SetTargetFPS(lua_State *L)
{
    int fps = LuaGetArgument_int(L, 1);
    SetTargetFPS(fps);
    return 0;
}

// Returns current FPS
int lua_GetFPS(lua_State *L)
{
    int result = GetFPS();
    LuaPush_int(L, result);
    return 1;
}

// Returns time in seconds for last frame drawn
int lua_GetFrameTime(lua_State *L)
{
    float result = GetFrameTime();
    LuaPush_float(L, result);
    return 1;
}

// Returns elapsed time in seconds since InitWindow()
int lua_GetTime(lua_State *L)
{
    double result = GetTime();
    LuaPush_double(L, result);
    return 1;
}

// Color-related functions
// Returns hexadecimal value for a Color
int lua_ColorToInt(lua_State *L)
{
    Color color = LuaGetArgument_Color(L, 1);
    int result = ColorToInt(color);
    LuaPush_int(L, result);
    return 1;
}

// Returns color normalized as float [0..1]
int lua_ColorNormalize(lua_State *L)
{
    Color color = LuaGetArgument_Color(L, 1);
    Vector4 result = ColorNormalize(color);
    LuaPush_Vector4(L, result);
    return 1;
}

// Returns HSV values for a Color
int lua_ColorToHSV(lua_State *L)
{
    Color color = LuaGetArgument_Color(L, 1);
    Vector3 result = ColorToHSV(color);
    LuaPush_Vector3(L, result);
    return 1;
}

// Returns a Color struct from hexadecimal value
int lua_GetColor(lua_State *L)
{
    int hexValue = LuaGetArgument_int(L, 1);
    Color result = GetColor(hexValue);
    LuaPush_Color(L, result);
    return 1;
}

// Color fade-in or fade-out, alpha goes from 0.0f to 1.0f
int lua_Fade(lua_State *L)
{
    Color color = LuaGetArgument_Color(L, 1);
    float alpha = LuaGetArgument_float(L, 2);
    Color result = Fade(color, alpha);
    LuaPush_Color(L, result);
    return 1;
}

#if 0
// Misc. functions
// Activate raylib logo at startup (can be done with flags)
int lua_ShowLogo(lua_State *L)
{
    ShowLogo();
    return 0;
}

// Setup window configuration flags (view FLAGS)
int lua_SetConfigFlags(lua_State *L)
{
    unsigned char flags = LuaGetArgument_unsigned(L, 1);
    SetConfigFlags(flags);
    return 0;
}

#endif

// Enable trace log message types (bit flags based)
int lua_SetTraceLogLevel(lua_State *L)
{
    int logLevel = LuaGetArgument_int(L, 1);
    SetTraceLogLevel(logLevel);
    return 0;
}


/*
#if defined(PLATFORM_WEB)
static int LuaDrawLoopFunc;

static void LuaDrawLoop()
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, LuaDrawLoopFunc);
    lua_call(L, 0, 0);
}

int lua_SetDrawingLoop(lua_State* L)
{
    luaL_argcheck(L, lua_isfunction(L, 1), 1, "Loop function expected");
    lua_pushvalue(L, 1);
    LuaDrawLoopFunc = luaL_ref(L, LUA_REGISTRYINDEX);
    SetDrawingLoop(&LuaDrawLoop);
    return 0;
}
#else
// Set target FPS (maximum)
int lua_SetTargetFPS(lua_State* L)
{
    int arg1 = LuaGetArgument_int(L, 1);
    SetTargetFPS(arg1);
    return 0;
}
#endif
*/


/*
// Converts Color to float array and normalizes
int lua_ColorToFloat(lua_State* L)
{
    Color arg1 = LuaGetArgument_Color(L, 1);
    float *result = ColorToFloat(arg1);
    lua_createtable(L, 4, 0);
    for (int i = 0; i < 4; i++)
    {
        LuaPush_float(L, result[i]);
        lua_rawseti(L, -2, i + 1);
    }
    free(result);
    return 1;
}

// Converts Vector3 to float array
int lua_VectorToFloat(lua_State* L)
{
    Vector3 arg1 = LuaGetArgument_Vector3(L, 1);
    float *result = VectorToFloat(arg1);
    lua_createtable(L, 3, 0);
    for (int i = 0; i < 3; i++)
    {
        LuaPush_float(L, result[i]);
        lua_rawseti(L, -2, i + 1);
    }
    free(result);
    return 1;
}

// Converts Matrix to float array
int lua_MatrixToFloat(lua_State* L)
{
    Matrix arg1 = LuaGetArgument_Matrix(L, 1);
    float *result = MatrixToFloat(arg1);
    lua_createtable(L, 16, 0);
    for (int i = 0; i < 16; i++)
    {
        LuaPush_float(L, result[i]);
        lua_rawseti(L, -2, i + 1);
    }
    free(result);
    return 1;
}
*/

// WARNING: Show trace log messages (INFO, WARNING, ERROR, DEBUG)
int lua_TraceLog(lua_State* L)
{
    int num_args = lua_gettop(L) - 1;
    int arg1 = LuaGetArgument_int(L, 1);

    /// type, fmt, args...

    lua_rotate(L, 1, -1); /// fmt, args..., type
    lua_pop(L, 1); /// fmt, args...

    lua_getglobal(L, "string"); /// fmt, args..., [string]
    lua_getfield(L, 1, "format"); /// fmt, args..., [string], format()
    lua_rotate(L, 1, 2); /// [string], format(), fmt, args...
    lua_call(L, num_args, 1); /// [string], formatted_string

    TraceLog(arg1, "%s", luaL_checkstring(L,-1));
    return 0;
}

// Takes a screenshot of current screen (saved a .png)
int lua_TakeScreenshot(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    TakeScreenshot(fileName);
    return 0;
}

// Returns a random value between min and max (both included)
int lua_GetRandomValue(lua_State *L)
{
    int min = LuaGetArgument_int(L, 1);
    int max = LuaGetArgument_int(L, 2);
    int result = GetRandomValue(min, max);
    LuaPush_int(L, result);
    return 1;
}

// Check file extension
int lua_IsFileExtension(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    const char *ext = LuaGetArgument_string(L, 2);
    bool result = IsFileExtension(fileName, ext);
    LuaPush_bool(L, result);
    return 1;
}

#if 0
// WARNING: Get pointer to extension for a filename string
int lua_GetExtension(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    const char *result = GetExtension(fileName);
    LuaPush_string(L, result);
    return 1;
}
#endif

// Get pointer to filename for a path string
int lua_GetFileName(lua_State *L)
{
    const char *filePath = LuaGetArgument_string(L, 1);
    const char * result = GetFileName(filePath);
    LuaPush_string(L, result);
    return 1;
}

// Get full path for a given fileName (uses static string)
int lua_GetDirectoryPath(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    const char * result = GetDirectoryPath(fileName);
    LuaPush_string(L, result);
    return 1;
}

// Get current working directory (uses static string)
int lua_GetWorkingDirectory(lua_State *L)
{
    const char * result = GetWorkingDirectory();
    LuaPush_string(L, result);
    return 1;
}

// Change working directory, returns true if success
int lua_ChangeDirectory(lua_State *L)
{
    const char *dir = LuaGetArgument_string(L, 1);
    bool result = ChangeDirectory(dir);
    LuaPush_bool(L, result);
    return 1;
}

// Check if a file has been dropped into window
int lua_IsFileDropped(lua_State* L)
{
    bool result = IsFileDropped();
    LuaPush_bool(L, result);
    return 1;
}

#if 0
// WARNING: Get dropped files names
int lua_GetDroppedFiles(lua_State* L)
{
    int count = 0;
    char **result = GetDroppedFiles(&count);
    lua_createtable(L, count, 0);
    for (int i = 0; i < count; i++)
    {
        LuaPush_string(L, result[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// Clear dropped files paths buffer
int lua_ClearDroppedFiles(lua_State* L)
{
    ClearDroppedFiles();
    return 0;
}

// Save integer value to storage file (to defined position)
int lua_StorageSaveValue(lua_State *L)
{
    int position = LuaGetArgument_int(L, 1);
    int value = LuaGetArgument_int(L, 2);
    StorageSaveValue(position, value);
    return 0;
}

// Load integer value from storage file (from defined position)
int lua_StorageLoadValue(lua_State *L)
{
    int position = LuaGetArgument_int(L, 1);
    int result = StorageLoadValue(position);
    LuaPush_int(L, result);
    return 1;
}
#endif

//------------------------------------------------------------------------------------
// raylib [core] module functions - Input Handling
//------------------------------------------------------------------------------------

// Detect if a key has been pressed once
int lua_IsKeyPressed(lua_State *L)
{
    int key = LuaGetArgument_int(L, 1);
    bool result = IsKeyPressed(key);
    LuaPush_bool(L, result);
    return 1;
}

// Detect if a key is being pressed
int lua_IsKeyDown(lua_State *L)
{
    int key = LuaGetArgument_int(L, 1);
    bool result = IsKeyDown(key);
    LuaPush_bool(L, result);
    return 1;
}

// Detect if a key has been released once
int lua_IsKeyReleased(lua_State *L)
{
    int key = LuaGetArgument_int(L, 1);
    bool result = IsKeyReleased(key);
    LuaPush_bool(L, result);
    return 1;
}

// Detect if a key is NOT being pressed
int lua_IsKeyUp(lua_State *L)
{
    int key = LuaGetArgument_int(L, 1);
    bool result = IsKeyUp(key);
    LuaPush_bool(L, result);
    return 1;
}

// Get latest key pressed
int lua_GetKeyPressed(lua_State *L)
{
    int result = GetKeyPressed();
    LuaPush_int(L, result);
    return 1;
}

// Get latest key pressed
int lua_GetCharPressed(lua_State *L)
{
    int result = GetCharPressed();
    LuaPush_int(L, result);
    return 1;
}

// Set a custom key to exit program (default is ESC)
int lua_SetExitKey(lua_State *L)
{
    int key = LuaGetArgument_int(L, 1);
    SetExitKey(key);
    return 0;
}

// Input-related functions: gamepads
// Detect if a gamepad is available
int lua_IsGamepadAvailable(lua_State *L)
{
    int gamepad = LuaGetArgument_int(L, 1);
    bool result = IsGamepadAvailable(gamepad);
    LuaPush_bool(L, result);
    return 1;
}

#if 0
// WARNING: Check gamepad name (if available)
int lua_IsGamepadName(lua_State* L)
{
    int arg1 = LuaGetArgument_int(L, 1);
    const char *arg2 = LuaGetArgument_string(L, 2);
    bool result = IsGamepadName(arg1, arg2);
    LuaPush_bool(L, result);
    return 1;
}
#endif

// WARNING: Return gamepad internal name id
int lua_GetGamepadName(lua_State* L)
{
    int arg1 = LuaGetArgument_int(L, 1);
    const char *result = GetGamepadName(arg1);
    LuaPush_string(L, result);
    return 1;
}

// Detect if a gamepad button has been pressed once
int lua_IsGamepadButtonPressed(lua_State *L)
{
    int gamepad = LuaGetArgument_int(L, 1);
    int button = LuaGetArgument_int(L, 2);
    bool result = IsGamepadButtonPressed(gamepad, button);
    LuaPush_bool(L, result);
    return 1;
}

// Detect if a gamepad button is being pressed
int lua_IsGamepadButtonDown(lua_State *L)
{
    int gamepad = LuaGetArgument_int(L, 1);
    int button = LuaGetArgument_int(L, 2);
    bool result = IsGamepadButtonDown(gamepad, button);
    LuaPush_bool(L, result);
    return 1;
}

// Detect if a gamepad button has been released once
int lua_IsGamepadButtonReleased(lua_State *L)
{
    int gamepad = LuaGetArgument_int(L, 1);
    int button = LuaGetArgument_int(L, 2);
    bool result = IsGamepadButtonReleased(gamepad, button);
    LuaPush_bool(L, result);
    return 1;
}

// Detect if a gamepad button is NOT being pressed
int lua_IsGamepadButtonUp(lua_State *L)
{
    int gamepad = LuaGetArgument_int(L, 1);
    int button = LuaGetArgument_int(L, 2);
    bool result = IsGamepadButtonUp(gamepad, button);
    LuaPush_bool(L, result);
    return 1;
}

// Get the last gamepad button pressed
int lua_GetGamepadButtonPressed(lua_State *L)
{
    int result = GetGamepadButtonPressed();
    LuaPush_int(L, result);
    return 1;
}

// Return gamepad axis count for a gamepad
int lua_GetGamepadAxisCount(lua_State *L)
{
    int gamepad = LuaGetArgument_int(L, 1);
    int result = GetGamepadAxisCount(gamepad);
    LuaPush_int(L, result);
    return 1;
}

// Return axis movement value for a gamepad axis
int lua_GetGamepadAxisMovement(lua_State *L)
{
    int gamepad = LuaGetArgument_int(L, 1);
    int axis = LuaGetArgument_int(L, 2);
    float result = GetGamepadAxisMovement(gamepad, axis);
    LuaPush_float(L, result);
    return 1;
}

// Input-related functions: mouse
// Detect if a mouse button has been pressed once
int lua_IsMouseButtonPressed(lua_State *L)
{
    int button = LuaGetArgument_int(L, 1);
    bool result = IsMouseButtonPressed(button);
    LuaPush_bool(L, result);
    return 1;
}

// Detect if a mouse button is being pressed
int lua_IsMouseButtonDown(lua_State *L)
{
    int button = LuaGetArgument_int(L, 1);
    bool result = IsMouseButtonDown(button);
    LuaPush_bool(L, result);
    return 1;
}

// Detect if a mouse button has been released once
int lua_IsMouseButtonReleased(lua_State *L)
{
    int button = LuaGetArgument_int(L, 1);
    bool result = IsMouseButtonReleased(button);
    LuaPush_bool(L, result);
    return 1;
}

// Detect if a mouse button is NOT being pressed
int lua_IsMouseButtonUp(lua_State *L)
{
    int button = LuaGetArgument_int(L, 1);
    bool result = IsMouseButtonUp(button);
    LuaPush_bool(L, result);
    return 1;
}

// Returns mouse position X
int lua_GetMouseX(lua_State *L)
{
    int result = GetMouseX();
    LuaPush_int(L, result);
    return 1;
}

// Returns mouse position Y
int lua_GetMouseY(lua_State *L)
{
    int result = GetMouseY();
    LuaPush_int(L, result);
    return 1;
}

// Returns mouse position XY
int lua_GetMousePosition(lua_State *L)
{
    Vector2 result = GetMousePosition();
    LuaPush_Vector2(L, result);
    return 1;
}

// Set mouse position XY
int lua_SetMousePosition(lua_State *L)
{
  //Vector2 position = LuaGetArgument_Vector2(L, 1);
  int x = LuaGetArgument_int(L, 1);
  int y = LuaGetArgument_int(L, 2);
  SetMousePosition(x, y);
    return 0;
}

// Set mouse scaling
int lua_SetMouseScale(lua_State *L)
{
    float scaleX = LuaGetArgument_float(L, 1);
    float scaleY = LuaGetArgument_float(L, 2);
    SetMouseScale(scaleX, scaleY);
    return 0;
}

// Returns mouse wheel movement Y
int lua_GetMouseWheelMove(lua_State *L)
{
    int result = GetMouseWheelMove();
    LuaPush_int(L, result);
    return 1;
}

// Input-related functions: touch
// Returns touch position X for touch point 0 (relative to screen size)
int lua_GetTouchX(lua_State *L)
{
    int result = GetTouchX();
    LuaPush_int(L, result);
    return 1;
}

// Returns touch position Y for touch point 0 (relative to screen size)
int lua_GetTouchY(lua_State *L)
{
    int result = GetTouchY();
    LuaPush_int(L, result);
    return 1;
}

// Returns touch position XY for a touch point index (relative to screen size)
int lua_GetTouchPosition(lua_State *L)
{
    int index = LuaGetArgument_int(L, 1);
    Vector2 result = GetTouchPosition(index);
    LuaPush_Vector2(L, result);
    return 1;
}

//------------------------------------------------------------------------------------
// raylib [gestures] module functions - Gestures and Touch Handling
//------------------------------------------------------------------------------------

// Enable a set of gestures using flags
int lua_SetGesturesEnabled(lua_State *L)
{
    unsigned int gestureFlags = LuaGetArgument_unsigned(L, 1);
    SetGesturesEnabled(gestureFlags);
    return 0;
}

// Check if a gesture have been detected
int lua_IsGestureDetected(lua_State *L)
{
    int gesture = LuaGetArgument_int(L, 1);
    bool result = IsGestureDetected(gesture);
    LuaPush_bool(L, result);
    return 1;
}

// Get latest detected gesture
int lua_GetGestureDetected(lua_State *L)
{
    int result = GetGestureDetected();
    LuaPush_int(L, result);
    return 1;
}

// Get touch points count
int lua_GetTouchPointCount(lua_State *L)
{
    int result = GetTouchPointCount();
    LuaPush_int(L, result);
    return 1;
}

// Get gesture hold time in milliseconds
int lua_GetGestureHoldDuration(lua_State *L)
{
    float result = GetGestureHoldDuration();
    LuaPush_float(L, result);
    return 1;
}

// Get gesture drag vector
int lua_GetGestureDragVector(lua_State *L)
{
    Vector2 result = GetGestureDragVector();
    LuaPush_Vector2(L, result);
    return 1;
}

// Get gesture drag angle
int lua_GetGestureDragAngle(lua_State *L)
{
    float result = GetGestureDragAngle();
    LuaPush_float(L, result);
    return 1;
}

// Get gesture pinch delta
int lua_GetGesturePinchVector(lua_State *L)
{
    Vector2 result = GetGesturePinchVector();
    LuaPush_Vector2(L, result);
    return 1;
}

// Get gesture pinch angle
int lua_GetGesturePinchAngle(lua_State *L)
{
    float result = GetGesturePinchAngle();
    LuaPush_float(L, result);
    return 1;
}

//------------------------------------------------------------------------------------
// raylib [camera] module functions - Camera System
//------------------------------------------------------------------------------------

// Update camera position for selected mode
int lua_UpdateCamera(lua_State *L)
{
    Camera camera = LuaGetArgument_Camera(L, 1);
    int mode = LuaGetArgument_int(L, 2);
    UpdateCamera(&camera, mode);
    LuaPush_Camera(L, camera);
    return 1;
}

#if 0

int lua_UpdateCameraPro(lua_State *L)
{
    Camera camera = LuaGetArgument_Camera(L, 1);
    int mode = LuaGetArgument_int(L, 2);
    UpdateCameraPro(&camera, mode);
    return 0;
}
#endif


//------------------------------------------------------------------------------------
// raylib [shapes] module functions - Basic Shapes Drawing
//------------------------------------------------------------------------------------

// Draw a pixel
int lua_DrawPixel(lua_State *L)
{
    int posX = LuaGetArgument_int(L, 1);
    int posY = LuaGetArgument_int(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawPixel(posX, posY, color);
    return 0;
}

// Draw a pixel (Vector version)
int lua_DrawPixelV(lua_State *L)
{
    Vector2 position = LuaGetArgument_Vector2(L, 1);
    Color color = LuaGetArgument_Color(L, 2);
    DrawPixelV(position, color);
    return 0;
}

// Draw a line
int lua_DrawLine(lua_State *L)
{
    int startPosX = LuaGetArgument_int(L, 1);
    int startPosY = LuaGetArgument_int(L, 2);
    int endPosX = LuaGetArgument_int(L, 3);
    int endPosY = LuaGetArgument_int(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawLine(startPosX, startPosY, endPosX, endPosY, color);
    return 0;
}

// Draw a line (Vector version)
int lua_DrawLineV(lua_State *L)
{
    Vector2 startPos = LuaGetArgument_Vector2(L, 1);
    Vector2 endPos = LuaGetArgument_Vector2(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawLineV(startPos, endPos, color);
    return 0;
}

// Draw a line defining thickness
int lua_DrawLineEx(lua_State *L)
{
    Vector2 startPos = LuaGetArgument_Vector2(L, 1);
    Vector2 endPos = LuaGetArgument_Vector2(L, 2);
    float thick = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawLineEx(startPos, endPos, thick, color);
    return 0;
}

int lua_DrawLineStrip(lua_State *L)
{
    GET_TABLE(Vector2, points, 1);
    int pointCount = LuaGetArgument_int(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawLineStrip(points, pointCount, color);

    free(points);
    return 0;
}

// Draw a line using cubic-bezier curves in-out
int lua_DrawLineBezier(lua_State *L)
{
    Vector2 startPos = LuaGetArgument_Vector2(L, 1);
    Vector2 endPos = LuaGetArgument_Vector2(L, 2);
    float thick = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawLineBezier(startPos, endPos, thick, color);
    return 0;
}

// Draw a color-filled circle
int lua_DrawCircle(lua_State *L)
{
    int centerX = LuaGetArgument_int(L, 1);
    int centerY = LuaGetArgument_int(L, 2);
    float radius = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawCircle(centerX, centerY, radius, color);
    return 0;
}
  
int lua_DrawCircleSector(lua_State *L)
{
    Vector2 center = LuaGetArgument_Vector2(L, 1);
    float radius = LuaGetArgument_float(L, 2);
    float startAngle = LuaGetArgument_float(L, 3);
    float endAngle = LuaGetArgument_float(L, 4);
    int segments = LuaGetArgument_int(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    DrawCircleSector(center, radius, startAngle, endAngle, segments, color);
    return 0;
}

int lua_DrawCircleSectorLines(lua_State *L)
{
    Vector2 center = LuaGetArgument_Vector2(L, 1);
    float radius = LuaGetArgument_float(L, 2);
    float startAngle = LuaGetArgument_float(L, 3);
    float endAngle = LuaGetArgument_float(L, 4);
    int segments = LuaGetArgument_int(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    DrawCircleSectorLines(center, radius, startAngle, endAngle, segments, color);
    return 0;
}

// Draw a gradient-filled circle
int lua_DrawCircleGradient(lua_State *L)
{
    int centerX = LuaGetArgument_int(L, 1);
    int centerY = LuaGetArgument_int(L, 2);
    float radius = LuaGetArgument_float(L, 3);
    Color color1 = LuaGetArgument_Color(L, 4);
    Color color2 = LuaGetArgument_Color(L, 5);
    DrawCircleGradient(centerX, centerY, radius, color1, color2);
    return 0;
}

// Draw a color-filled circle (Vector version)
int lua_DrawCircleV(lua_State *L)
{
    Vector2 center = LuaGetArgument_Vector2(L, 1);
    float radius = LuaGetArgument_float(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawCircleV(center, radius, color);
    return 0;
}

// Draw circle outline
int lua_DrawCircleLines(lua_State *L)
{
    int centerX = LuaGetArgument_int(L, 1);
    int centerY = LuaGetArgument_int(L, 2);
    float radius = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawCircleLines(centerX, centerY, radius, color);
    return 0;
}

// Draw circle outline
int lua_DrawCircleLinesV(lua_State *L)
{
    Vector2 center = LuaGetArgument_Vector2(L, 1);
    float radius = LuaGetArgument_float(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawCircleLinesV(center, radius, color);
    return 0;
}

int lua_DrawEllipse(lua_State *L)
{
  int centerX = LuaGetArgument_int(L, 1);
  int centerY = LuaGetArgument_int(L, 2);
  float radiusH = LuaGetArgument_float(L, 3);
  float radiusV = LuaGetArgument_float(L, 4);
  Color color = LuaGetArgument_Color(L, 5);
  DrawEllipse(centerX, centerY, radiusH, radiusV, color);
  return 0;  
}

int lua_DrawEllipseLines(lua_State *L)
{
  int centerX = LuaGetArgument_int(L, 1);
  int centerY = LuaGetArgument_int(L, 2);
  float radiusH = LuaGetArgument_float(L, 3);
  float radiusV = LuaGetArgument_float(L, 4);
  Color color = LuaGetArgument_Color(L, 5);
  DrawEllipseLines(centerX, centerY, radiusH, radiusV, color);
  return 0;  
}

int lua_DrawRing(lua_State *L)
{
  Vector2 center = LuaGetArgument_Vector2(L, 1);
  float innerRadius = LuaGetArgument_float(L, 2);
  float outerRadius = LuaGetArgument_float(L, 3);
  float startAngle = LuaGetArgument_float(L, 4);
  float endAngle = LuaGetArgument_float(L, 5);
  int segments = LuaGetArgument_int(L, 6);
  Color color = LuaGetArgument_Color(L, 7);
  DrawRing(center, innerRadius, outerRadius, startAngle, endAngle, segments, color);
  return 0;
}

int lua_DrawRingLines(lua_State *L)
{
  Vector2 center = LuaGetArgument_Vector2(L, 1);
  float innerRadius = LuaGetArgument_float(L, 2);
  float outerRadius = LuaGetArgument_float(L, 3);
  float startAngle = LuaGetArgument_float(L, 4);
  float endAngle = LuaGetArgument_float(L, 5);
  int segments = LuaGetArgument_int(L, 6);
  Color color = LuaGetArgument_Color(L, 7);
  DrawRingLines(center, innerRadius, outerRadius, startAngle, endAngle, segments, color);
  return 0;
}

// Draw a color-filled rectangle
int lua_DrawRectangle(lua_State *L)
{
    int posX = LuaGetArgument_int(L, 1);
    int posY = LuaGetArgument_int(L, 2);
    int width = LuaGetArgument_int(L, 3);
    int height = LuaGetArgument_int(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawRectangle(posX, posY, width, height, color);
    return 0;
}

// Draw a color-filled rectangle (Vector version)
int lua_DrawRectangleV(lua_State *L)
{
    Vector2 position = LuaGetArgument_Vector2(L, 1);
    Vector2 size = LuaGetArgument_Vector2(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawRectangleV(position, size, color);
    return 0;
}

// Draw a color-filled rectangle
int lua_DrawRectangleRec(lua_State *L)
{
    Rectangle rec = LuaGetArgument_Rectangle(L, 1);
    Color color = LuaGetArgument_Color(L, 2);
    DrawRectangleRec(rec, color);
    return 0;
}

// Draw a color-filled rectangle with pro parameters
int lua_DrawRectanglePro(lua_State *L)
{
    Rectangle rec = LuaGetArgument_Rectangle(L, 1);
    Vector2 origin = LuaGetArgument_Vector2(L, 2);
    float rotation = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawRectanglePro(rec, origin, rotation, color);
    return 0;
}

// Draw a vertical-gradient-filled rectangle
int lua_DrawRectangleGradientV(lua_State *L)
{
    int posX = LuaGetArgument_int(L, 1);
    int posY = LuaGetArgument_int(L, 2);
    int width = LuaGetArgument_int(L, 3);
    int height = LuaGetArgument_int(L, 4);
    Color color1 = LuaGetArgument_Color(L, 5);
    Color color2 = LuaGetArgument_Color(L, 6);
    DrawRectangleGradientV(posX, posY, width, height, color1, color2);
    return 0;
}

// Draw a horizontal-gradient-filled rectangle
int lua_DrawRectangleGradientH(lua_State *L)
{
    int posX = LuaGetArgument_int(L, 1);
    int posY = LuaGetArgument_int(L, 2);
    int width = LuaGetArgument_int(L, 3);
    int height = LuaGetArgument_int(L, 4);
    Color color1 = LuaGetArgument_Color(L, 5);
    Color color2 = LuaGetArgument_Color(L, 6);
    DrawRectangleGradientH(posX, posY, width, height, color1, color2);
    return 0;
}

// Draw a gradient-filled rectangle with custom vertex colors
int lua_DrawRectangleGradientEx(lua_State *L)
{
    Rectangle rec = LuaGetArgument_Rectangle(L, 1);
    Color col1 = LuaGetArgument_Color(L, 2);
    Color col2 = LuaGetArgument_Color(L, 3);
    Color col3 = LuaGetArgument_Color(L, 4);
    Color col4 = LuaGetArgument_Color(L, 5);
    DrawRectangleGradientEx(rec, col1, col2, col3, col4);
    return 0;
}

// Draw rectangle outline
int lua_DrawRectangleLines(lua_State *L)
{
    int posX = LuaGetArgument_int(L, 1);
    int posY = LuaGetArgument_int(L, 2);
    int width = LuaGetArgument_int(L, 3);
    int height = LuaGetArgument_int(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawRectangleLines(posX, posY, width, height, color);
    return 0;
}

// Draw rectangle outline with extended parameters
int lua_DrawRectangleLinesEx(lua_State *L)
{
    Rectangle rec = LuaGetArgument_Rectangle(L, 1);
    float lineThick = LuaGetArgument_float(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawRectangleLinesEx(rec, lineThick, color);
    return 0;
}

int lua_DrawRectangleRounded(lua_State *L)
{
    Rectangle rec = LuaGetArgument_Rectangle(L, 1);
    float roundness = LuaGetArgument_float(L, 2);
    int segments = LuaGetArgument_int(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawRectangleRounded(rec, roundness, segments, color);
    return 0;
}

int lua_DrawRectangleRoundedLines(lua_State *L)
{
    Rectangle rec = LuaGetArgument_Rectangle(L, 1);
    float roundness = LuaGetArgument_float(L, 2);
    int segments = LuaGetArgument_int(L, 3);
    float lineThick = LuaGetArgument_float(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawRectangleRoundedLines(rec, roundness, segments, lineThick, color);
    return 0;
}

// Draw a color-filled triangle
int lua_DrawTriangle(lua_State *L)
{
    Vector2 v1 = LuaGetArgument_Vector2(L, 1);
    Vector2 v2 = LuaGetArgument_Vector2(L, 2);
    Vector2 v3 = LuaGetArgument_Vector2(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawTriangle(v1, v2, v3, color);
    return 0;
}

// Draw triangle outline
int lua_DrawTriangleLines(lua_State *L)
{
    Vector2 v1 = LuaGetArgument_Vector2(L, 1);
    Vector2 v2 = LuaGetArgument_Vector2(L, 2);
    Vector2 v3 = LuaGetArgument_Vector2(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawTriangleLines(v1, v2, v3, color);
    return 0;
}

int lua_DrawTriangleFan(lua_State *L)
{
    GET_TABLE(Vector2, points, 1);
    int pointCount = LuaGetArgument_int(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawTriangleFan(points, pointCount, color);

    free(points);
    return 0;
}

int lua_DrawTriangleStrip(lua_State *L)
{
    GET_TABLE(Vector2, points, 1);
    int pointCount = LuaGetArgument_int(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawTriangleStrip(points, pointCount, color);
    
    free(points);
    return 0;
}

// Draw a regular polygon (Vector version)
int lua_DrawPoly(lua_State *L)
{
    Vector2 center = LuaGetArgument_Vector2(L, 1);
    int sides = LuaGetArgument_int(L, 2);
    float radius = LuaGetArgument_float(L, 3);
    float rotation = LuaGetArgument_float(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawPoly(center, sides, radius, rotation, color);
    return 0;
}

// Draw a regular polygon outline (Vector version)
int lua_DrawPolyLines(lua_State *L)
{
    Vector2 center = LuaGetArgument_Vector2(L, 1);
    int sides = LuaGetArgument_int(L, 2);
    float radius = LuaGetArgument_float(L, 3);
    float rotation = LuaGetArgument_float(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawPolyLines(center, sides, radius, rotation, color);
    return 0;
}

// Draw a regular polygon outline (Vector version)
int lua_DrawPolyLinesEx(lua_State *L)
{
    Vector2 center = LuaGetArgument_Vector2(L, 1);
    int sides = LuaGetArgument_int(L, 2);
    float radius = LuaGetArgument_float(L, 3);
    float rotation = LuaGetArgument_float(L, 4);
    float lineThick = LuaGetArgument_float(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    DrawPolyLinesEx(center, sides, radius, rotation, lineThick, color);
    return 0;
}

int lua_DrawSplineLinear(lua_State *L)
{
    GET_TABLE(Vector2, points, 1);
    int pointCount = LuaGetArgument_int(L, 2);
    float thick = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawSplineLinear(points, pointCount, thick, color);
    
    free(points);
    return 0;
}

int lua_DrawSplineBasis(lua_State *L)
{
    GET_TABLE(Vector2, points, 1);
    int pointCount = LuaGetArgument_int(L, 2);
    float thick = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawSplineBasis(points, pointCount, thick, color);
    
    free(points);
    return 0;
}

int lua_DrawSplineCatmullRom(lua_State *L)
{
    GET_TABLE(Vector2, points, 1);
    int pointCount = LuaGetArgument_int(L, 2);
    float thick = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawSplineCatmullRom(points, pointCount, thick, color);
    
    free(points);
    return 0;
}

int lua_DrawSplineBezierQuadratic(lua_State *L)
{
    GET_TABLE(Vector2, points, 1);
    int pointCount = LuaGetArgument_int(L, 2);
    float thick = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawSplineBezierQuadratic(points, pointCount, thick, color);
    
    free(points);
    return 0;
}

int lua_DrawSplineBezierCubic(lua_State *L)
{
    GET_TABLE(Vector2, points, 1);
    int pointCount = LuaGetArgument_int(L, 2);
    float thick = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawSplineBezierCubic(points, pointCount, thick, color);
    
    free(points);
    return 0;
}

int lua_DrawSplineSegmentLinear(lua_State *L)
{
    Vector2 p1 = LuaGetArgument_Vector2(L, 1);
    Vector2 p2 = LuaGetArgument_Vector2(L, 2);
    float thick = LuaGetArgument_float(L, 3);
    Color color = LuaGetArgument_Color(L, 4);
    DrawSplineSegmentLinear(p1, p2, thick, color);
    return 0;
}

int lua_DrawSplineSegmentBasis(lua_State *L)
{
    Vector2 p1 = LuaGetArgument_Vector2(L, 1);
    Vector2 p2 = LuaGetArgument_Vector2(L, 2);
    Vector2 p3 = LuaGetArgument_Vector2(L, 3);
    Vector2 p4 = LuaGetArgument_Vector2(L, 4);
    float thick = LuaGetArgument_float(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    DrawSplineSegmentBasis(p1, p2, p3, p4, thick, color);
    return 0;
}

int lua_DrawSplineSegmentCatmullRom(lua_State *L)
{
    Vector2 p1 = LuaGetArgument_Vector2(L, 1);
    Vector2 p2 = LuaGetArgument_Vector2(L, 2);
    Vector2 p3 = LuaGetArgument_Vector2(L, 3);
    Vector2 p4 = LuaGetArgument_Vector2(L, 4);
    float thick = LuaGetArgument_float(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    DrawSplineSegmentCatmullRom(p1, p2, p3, p4, thick, color);
    return 0;
}

int lua_DrawSplineSegmentBezierQuadratic(lua_State *L)
{
    Vector2 p1 = LuaGetArgument_Vector2(L, 1);
    Vector2 c2 = LuaGetArgument_Vector2(L, 2);
    Vector2 p3 = LuaGetArgument_Vector2(L, 3);
    float thick = LuaGetArgument_float(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawSplineSegmentBezierQuadratic(p1, c2, p3, thick, color);
    return 0;
}

int lua_DrawSplineSegmentBezierCubic(lua_State *L)
{
    Vector2 p1 = LuaGetArgument_Vector2(L, 1);
    Vector2 c2 = LuaGetArgument_Vector2(L, 2);
    Vector2 c3 = LuaGetArgument_Vector2(L, 3);
    Vector2 p4 = LuaGetArgument_Vector2(L, 4);
    float thick = LuaGetArgument_float(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    DrawSplineSegmentBezierCubic(p1, c2, c3, p4, thick, color);
    return 0;
}


// Check collision between two rectangles
int lua_CheckCollisionRecs(lua_State *L)
{
    Rectangle rec1 = LuaGetArgument_Rectangle(L, 1);
    Rectangle rec2 = LuaGetArgument_Rectangle(L, 2);
    bool result = CheckCollisionRecs(rec1, rec2);
    LuaPush_bool(L, result);
    return 1;
}

// Check collision between two circles
int lua_CheckCollisionCircles(lua_State *L)
{
    Vector2 center1 = LuaGetArgument_Vector2(L, 1);
    float radius1 = LuaGetArgument_float(L, 2);
    Vector2 center2 = LuaGetArgument_Vector2(L, 3);
    float radius2 = LuaGetArgument_float(L, 4);
    bool result = CheckCollisionCircles(center1, radius1, center2, radius2);
    LuaPush_bool(L, result);
    return 1;
}

// Check collision between circle and rectangle
int lua_CheckCollisionCircleRec(lua_State *L)
{
    Vector2 center = LuaGetArgument_Vector2(L, 1);
    float radius = LuaGetArgument_float(L, 2);
    Rectangle rec = LuaGetArgument_Rectangle(L, 3);
    bool result = CheckCollisionCircleRec(center, radius, rec);
    LuaPush_bool(L, result);
    return 1;
}

// Get collision rectangle for two rectangles collision
int lua_GetCollisionRec(lua_State *L)
{
    Rectangle rec1 = LuaGetArgument_Rectangle(L, 1);
    Rectangle rec2 = LuaGetArgument_Rectangle(L, 2);
    Rectangle result = GetCollisionRec(rec1, rec2);
    LuaPush_Rectangle(L, result);
    return 1;
}

// Check if point is inside rectangle
int lua_CheckCollisionPointRec(lua_State *L)
{
    Vector2 point = LuaGetArgument_Vector2(L, 1);
    Rectangle rec = LuaGetArgument_Rectangle(L, 2);
    bool result = CheckCollisionPointRec(point, rec);
    LuaPush_bool(L, result);
    return 1;
}

// Check if point is inside circle
int lua_CheckCollisionPointCircle(lua_State *L)
{
    Vector2 point = LuaGetArgument_Vector2(L, 1);
    Vector2 center = LuaGetArgument_Vector2(L, 2);
    float radius = LuaGetArgument_float(L, 3);
    bool result = CheckCollisionPointCircle(point, center, radius);
    LuaPush_bool(L, result);
    return 1;
}

// Check if point is inside a triangle
int lua_CheckCollisionPointTriangle(lua_State *L)
{
    Vector2 point = LuaGetArgument_Vector2(L, 1);
    Vector2 p1 = LuaGetArgument_Vector2(L, 2);
    Vector2 p2 = LuaGetArgument_Vector2(L, 3);
    Vector2 p3 = LuaGetArgument_Vector2(L, 4);
    bool result = CheckCollisionPointTriangle(point, p1, p2, p3);
    LuaPush_bool(L, result);
    return 1;
}

//------------------------------------------------------------------------------------
// raylib [textures] module functions - Texture Loading and Drawing
//------------------------------------------------------------------------------------

// Load image from file into CPU memory (RAM)
int lua_LoadImage(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Image result = LoadImage(fileName);
    LuaPush_Image(L, result);
    return 1;
}

#if 0
// WARNING: Load image from Color array data (RGBA - 32bit)
int lua_LoadImageEx(lua_State* L)
{
    // TODO: arg1 parameter is a Color array...
    
    GET_TABLE(Color, arg1, 1);              // Color *pixels
    int arg2 = LuaGetArgument_int(L, 2);
    int arg3 = LuaGetArgument_int(L, 3);
    Image result = LoadImageEx(arg1, arg2, arg3); // ISSUE: #3 number expected, got no value
    LuaPush_Image(L, result);
    free(arg1);
    return 1;
}

// WARNING: Load image from raw data with pro parameters
int lua_LoadImagePro(lua_State* L)
{
    // TODO: arg1 parameter is a void pointer...
    
    void *arg1 = LuaGetArgument_ptr(L, 1);
    int arg2 = LuaGetArgument_int(L, 2);
    int arg3 = LuaGetArgument_int(L, 3);
    int arg4 = LuaGetArgument_int(L, 4);
    Image result = LoadImagePro(arg1, arg2, arg3, arg4);
    LuaPush_Image(L, result);
    free(arg1);
    return 1;
}
#endif

// Load image from RAW file data
int lua_LoadImageRaw(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    int width = LuaGetArgument_int(L, 2);
    int height = LuaGetArgument_int(L, 3);
    int format = LuaGetArgument_int(L, 4);
    int headerSize = LuaGetArgument_int(L, 5);
    Image result = LoadImageRaw(fileName, width, height, format, headerSize);
    LuaPush_Image(L, result);
    return 1;
}

// Export image as a PNG file
int lua_ExportImage(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Image image = LuaGetArgument_Image(L, 2);
    ExportImage(image, fileName);
    return 0;
}

// Load texture from file into GPU memory (VRAM)
int lua_LoadTexture(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Texture2D result = LoadTexture(fileName);
    LuaPush_Texture2D(L, result);
    return 1;
}

// Load texture from image data
int lua_LoadTextureFromImage(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    Texture2D result = LoadTextureFromImage(image);
    LuaPush_Texture2D(L, result);
    return 1;
}

// Load texture for rendering (framebuffer)
int lua_LoadRenderTexture(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    RenderTexture2D result = LoadRenderTexture(width, height);
    LuaPush_RenderTexture2D(L, result);
    return 1;
}

// Unload image from CPU memory (RAM)
int lua_UnloadImage(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    UnloadImage(image);
    return 0;
}

// Unload texture from GPU memory (VRAM)
int lua_UnloadTexture(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    UnloadTexture(texture);
    return 0;
}

// Unload render texture from GPU memory (VRAM)
int lua_UnloadRenderTexture(lua_State *L)
{
    RenderTexture2D target = LuaGetArgument_RenderTexture2D(L, 1);
    UnloadRenderTexture(target);
    return 0;
}

#if 0
// WARNING: Get pixel data from image as a Color struct array
int lua_GetImageData(lua_State* L)
{
    // TODO: return value is a Color array

    Image arg1 = LuaGetArgument_Image(L, 1);
    Color *result = GetImageData(arg1);
    lua_createtable(L, arg1.width*arg1.height, 0);
    for (int i = 0; i < arg1.width*arg1.height; i++)
    {
        LuaPush_Color(L, result[i]);
        lua_rawseti(L, -2, i + 1);
    }
    free(result);
    return 1;
}

// WARNING: Get pixel data from image as Vector4 array (float normalized)
int lua_GetImageDataNormalized(lua_State *L)
{
    // TODO.
    
    return 1;
}
#endif

// Get pixel data size in bytes (image or texture)
int lua_GetPixelDataSize(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    int format = LuaGetArgument_int(L, 3);
    int result = GetPixelDataSize(width, height, format);
    LuaPush_int(L, result);
    return 1;
}

#if 0
// Get pixel data from GPU texture and return an Image
int lua_GetTextureData(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    Image result = GetTextureData(texture);
    LuaPush_Image(L, result);
    return 1;
}
#endif

// WARNING: Update GPU texture with new data
int lua_UpdateTexture(lua_State* L)
{
    // TODO: arg2 parameters is a void pointer...
    
    Texture2D arg1 = LuaGetArgument_Texture2D(L, 1);
    void *arg2 = LuaGetArgument_ptr(L, 2);
    UpdateTexture(arg1, arg2);      // ISSUE: #2 string expected, got table -> GetImageData() returns a table!
    return 0;
}

//----------------------------------------------------------------------------------
// Image manipulation functions
//----------------------------------------------------------------------------------

// Create an image duplicate (useful for transformations)
int lua_ImageCopy(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    Image result = ImageCopy(image);
    LuaPush_Image(L, result);
    return 1;
}

// Convert image to POT (power-of-two)
int lua_ImageToPOT(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    Color fillColor = LuaGetArgument_Color(L, 2);
    ImageToPOT(&image, fillColor);
    return 0;
}

// Convert image data to desired format
int lua_ImageFormat(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    int newFormat = LuaGetArgument_int(L, 2);
    ImageFormat(&image, newFormat);
    return 0;
}

// Apply alpha mask to image
int lua_ImageAlphaMask(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    Image alphaMask = LuaGetArgument_Image(L, 2);
    ImageAlphaMask(&image, alphaMask);
    return 0;
}

// Clear alpha channel to desired color
int lua_ImageAlphaClear(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    Color color = LuaGetArgument_Color(L, 2);
    float threshold = LuaGetArgument_float(L, 3);
    ImageAlphaClear(&image, color, threshold);
    return 0;
}

// Crop image depending on alpha value
int lua_ImageAlphaCrop(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    float threshold = LuaGetArgument_float(L, 2);
    ImageAlphaCrop(&image, threshold);
    return 0;
}

// Premultiply alpha channel
int lua_ImageAlphaPremultiply(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    ImageAlphaPremultiply(&image);
    return 0;
}

// Crop an image to a defined rectangle
int lua_ImageCrop(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    Rectangle crop = LuaGetArgument_Rectangle(L, 2);
    ImageCrop(&image, crop);
    return 0;
}

// Resize image (bilinear filtering)
int lua_ImageResize(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    int newWidth = LuaGetArgument_int(L, 2);
    int newHeight = LuaGetArgument_int(L, 3);
    ImageResize(&image, newWidth, newHeight);
    return 0;
}

// Resize image (Nearest-Neighbor scaling algorithm)
int lua_ImageResizeNN(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    int newWidth = LuaGetArgument_int(L, 2);
    int newHeight = LuaGetArgument_int(L, 3);
    ImageResizeNN(&image, newWidth, newHeight);
    return 0;
}

// Resize canvas and fill with color
int lua_ImageResizeCanvas(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    int newWidth = LuaGetArgument_int(L, 2);
    int newHeight = LuaGetArgument_int(L, 3);
    int offsetX = LuaGetArgument_int(L, 4);
    int offsetY = LuaGetArgument_int(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    ImageResizeCanvas(&image, newWidth, newHeight, offsetX, offsetY, color);
    return 0;
}

// Generate all mipmap levels for a provided image
int lua_ImageMipmaps(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    ImageMipmaps(&image);
    return 0;
}

// Dither image data to 16bpp or lower (Floyd-Steinberg dithering)
int lua_ImageDither(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    int rBpp = LuaGetArgument_int(L, 2);
    int gBpp = LuaGetArgument_int(L, 3);
    int bBpp = LuaGetArgument_int(L, 4);
    int aBpp = LuaGetArgument_int(L, 5);
    ImageDither(&image, rBpp, gBpp, bBpp, aBpp);
    return 0;
}

// Create an image from text (default font)
int lua_ImageText(lua_State *L)
{
    const char *text = LuaGetArgument_string(L, 1);
    int fontSize = LuaGetArgument_int(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    Image result = ImageText(text, fontSize, color);
    LuaPush_Image(L, result);
    return 1;
}

// Create an image from text (custom sprite font)
int lua_ImageTextEx(lua_State *L)
{
    Font font = LuaGetArgument_Font(L, 1);
    const char *text = LuaGetArgument_string(L, 2);
    float fontSize = LuaGetArgument_float(L, 3);
    float spacing = LuaGetArgument_float(L, 4);
    Color tint = LuaGetArgument_Color(L, 5);
    Image result = ImageTextEx(font, text, fontSize, spacing, tint);
    LuaPush_Image(L, result);
    return 1;
}

// Draw a source image within a destination image
int lua_ImageDraw(lua_State *L)
{
    Image dst = LuaGetArgument_Image(L, 1);
    Image src = LuaGetArgument_Image(L, 2);
    Rectangle srcRec = LuaGetArgument_Rectangle(L, 3);
    Rectangle dstRec = LuaGetArgument_Rectangle(L, 4);
    Color tint = LuaGetArgument_Color(L, 5);
    ImageDraw(&dst, src, srcRec, dstRec, tint);                   // WARNING: & required!
    return 0;
}

// Draw rectangle within an image
int lua_ImageDrawRectangle(lua_State *L)
{
    Image dst = LuaGetArgument_Image(L, 1);
    int posX = LuaGetArgument_int(L, 2);
    int posY = LuaGetArgument_int(L, 3);
    int width = LuaGetArgument_int(L, 4);
    int height = LuaGetArgument_int(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    ImageDrawRectangle(&dst, posX, posY, width, height, color);
    return 0;
}

// Draw text (default font) within an image (destination)
int lua_ImageDrawText(lua_State *L)
{
    Image dst = LuaGetArgument_Image(L, 1);
    const char *text = LuaGetArgument_string(L, 2);         // WARNING: all const char require *
    int posX = LuaGetArgument_int(L, 3);
    int posY = LuaGetArgument_int(L, 4);
    int fontSize = LuaGetArgument_int(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    ImageDrawText(&dst, text, posX, posY, fontSize, color);
    return 0;
}

// Draw text (custom sprite font) within an image (destination)
int lua_ImageDrawTextEx(lua_State *L)
{
    Image dst = LuaGetArgument_Image(L, 1);
    Font font = LuaGetArgument_Font(L, 2);
    const char *text = LuaGetArgument_string(L, 3);
    Vector2 position = LuaGetArgument_Vector2(L, 4);
    float fontSize = LuaGetArgument_float(L, 5);
    float spacing = LuaGetArgument_float(L, 6);
    Color color = LuaGetArgument_Color(L, 7);
    ImageDrawTextEx(&dst, font, text, position, fontSize, spacing, color);
    return 0;
}

// Flip image vertically
int lua_ImageFlipVertical(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    ImageFlipVertical(&image);
    return 0;
}

// Flip image horizontally
int lua_ImageFlipHorizontal(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    ImageFlipHorizontal(&image);
    return 0;
}

// Rotate image clockwise 90deg
int lua_ImageRotateCW(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    ImageRotateCW(&image);
    return 0;
}

// Rotate image counter-clockwise 90deg
int lua_ImageRotateCCW(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    ImageRotateCCW(&image);
    return 0;
}

// Modify image color: tint
int lua_ImageColorTint(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    Color color = LuaGetArgument_Color(L, 2);
    ImageColorTint(&image, color);
    return 0;
}

// Modify image color: invert
int lua_ImageColorInvert(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    ImageColorInvert(&image);
    return 0;
}

// Modify image color: grayscale
int lua_ImageColorGrayscale(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    ImageColorGrayscale(&image);
    return 0;
}

// Modify image color: contrast (-100 to 100)
int lua_ImageColorContrast(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    float contrast = LuaGetArgument_float(L, 2);
    ImageColorContrast(&image, contrast);
    return 0;
}

// Modify image color: brightness (-255 to 255)
int lua_ImageColorBrightness(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    int brightness = LuaGetArgument_int(L, 2);
    ImageColorBrightness(&image, brightness);
    return 0;
}

// Modify image color: replace color
int lua_ImageColorReplace(lua_State *L)
{
    Image image = LuaGetArgument_Image(L, 1);
    Color color = LuaGetArgument_Color(L, 2);
    Color replace = LuaGetArgument_Color(L, 3);
    ImageColorReplace(&image, color, replace);
    return 0;
}

// Image generation functions
// Generate image: plain color
int lua_GenImageColor(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    Image result = GenImageColor(width, height, color);
    LuaPush_Image(L, result);
    return 1;
}

#if 0
// Generate image: vertical gradient
int lua_GenImageGradientV(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    Color top = LuaGetArgument_Color(L, 3);
    Color bottom = LuaGetArgument_Color(L, 4);
    Image result = GenImageGradientV(width, height, top, bottom);
    LuaPush_Image(L, result);
    return 1;
}

// Generate image: horizontal gradient
int lua_GenImageGradientH(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    Color left = LuaGetArgument_Color(L, 3);
    Color right = LuaGetArgument_Color(L, 4);
    Image result = GenImageGradientH(width, height, left, right);
    LuaPush_Image(L, result);
    return 1;
}
#endif

// Generate image: radial gradient
int lua_GenImageGradientRadial(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    float density = LuaGetArgument_float(L, 3);
    Color inner = LuaGetArgument_Color(L, 4);
    Color outer = LuaGetArgument_Color(L, 5);
    Image result = GenImageGradientRadial(width, height, density, inner, outer);
    LuaPush_Image(L, result);
    return 1;
}

// Generate image: checked
int lua_GenImageChecked(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    int checksX = LuaGetArgument_int(L, 3);
    int checksY = LuaGetArgument_int(L, 4);
    Color col1 = LuaGetArgument_Color(L, 5);
    Color col2 = LuaGetArgument_Color(L, 6);
    Image result = GenImageChecked(width, height, checksX, checksY, col1, col2);
    LuaPush_Image(L, result);
    return 1;
}

// Generate image: white noise
int lua_GenImageWhiteNoise(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    float factor = LuaGetArgument_float(L, 3);
    Image result = GenImageWhiteNoise(width, height, factor);
    LuaPush_Image(L, result);
    return 1;
}

// Generate image: perlin noise
int lua_GenImagePerlinNoise(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    int offsetX = LuaGetArgument_int(L, 3);
    int offsetY = LuaGetArgument_int(L, 4);
    float scale = LuaGetArgument_float(L, 5);
    Image result = GenImagePerlinNoise(width, height, offsetX, offsetY, scale);
    LuaPush_Image(L, result);
    return 1;
}

// Generate image: cellular algorithm. Bigger tileSize means bigger cells
int lua_GenImageCellular(lua_State *L)
{
    int width = LuaGetArgument_int(L, 1);
    int height = LuaGetArgument_int(L, 2);
    int tileSize = LuaGetArgument_int(L, 3);
    Image result = GenImageCellular(width, height, tileSize);
    LuaPush_Image(L, result);
    return 1;
}

// Texture2D configuration functions

// Generate GPU mipmaps for a texture
int lua_GenTextureMipmaps(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    GenTextureMipmaps(&texture);
    return 0;
}

// Set texture scaling filter mode
int lua_SetTextureFilter(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    int filterMode = LuaGetArgument_int(L, 2);
    SetTextureFilter(texture, filterMode);
    return 0;
}

// Set texture wrapping mode
int lua_SetTextureWrap(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    int wrapMode = LuaGetArgument_int(L, 2);
    SetTextureWrap(texture, wrapMode);
    return 0;
}

// Texture2D drawing functions

// Draw a Texture2D
int lua_DrawTexture(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    int posX = LuaGetArgument_int(L, 2);
    int posY = LuaGetArgument_int(L, 3);
    Color tint = LuaGetArgument_Color(L, 4);
    DrawTexture(texture, posX, posY, tint);
    return 0;
}

// Draw a Texture2D with position defined as Vector2
int lua_DrawTextureV(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    Vector2 position = LuaGetArgument_Vector2(L, 2);
    Color tint = LuaGetArgument_Color(L, 3);
    DrawTextureV(texture, position, tint);
    return 0;
}

// Draw a Texture2D with extended parameters
int lua_DrawTextureEx(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    Vector2 position = LuaGetArgument_Vector2(L, 2);
    float rotation = LuaGetArgument_float(L, 3);
    float scale = LuaGetArgument_float(L, 4);
    Color tint = LuaGetArgument_Color(L, 5);
    DrawTextureEx(texture, position, rotation, scale, tint);
    return 0;
}

// Draw a part of a texture defined by a rectangle
int lua_DrawTextureRec(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    Rectangle sourceRec = LuaGetArgument_Rectangle(L, 2);
    Vector2 position = LuaGetArgument_Vector2(L, 3);
    Color tint = LuaGetArgument_Color(L, 4);
    DrawTextureRec(texture, sourceRec, position, tint);
    return 0;
}

// Draw a part of a texture defined by a rectangle with 'pro' parameters
int lua_DrawTexturePro(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    Rectangle sourceRec = LuaGetArgument_Rectangle(L, 2);
    Rectangle destRec = LuaGetArgument_Rectangle(L, 3);
    Vector2 origin = LuaGetArgument_Vector2(L, 4);
    float rotation = LuaGetArgument_float(L, 5);
    Color tint = LuaGetArgument_Color(L, 6);
    DrawTexturePro(texture, sourceRec, destRec, origin, rotation, tint);
    return 0;
}

//------------------------------------------------------------------------------------
// raylib [text] module functions - Font Loading and Text Drawing
//------------------------------------------------------------------------------------

// Get the default Font
int lua_GetFontDefault(lua_State *L)
{
    Font result = GetFontDefault();
    LuaPush_Font(L, result);
    return 1;
}

// Load font from file into GPU memory (VRAM)
int lua_LoadFont(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Font result = LoadFont(fileName);
    LuaPush_Font(L, result);
    return 1;
}

#if 0
// Load font from file with extended parameters
int lua_LoadFontEx(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    int fontSize = LuaGetArgument_int(L, 2);
    int charsCount = LuaGetArgument_int(L, 3);
    int fontChars = LuaGetArgument_int(L, 4);
    Font result = LoadFontEx(fileName, fontSize, charsCount, fontChars);
    LuaPush_Font(L, result);
    return 1;
}

// Load font data for further use
int lua_LoadFontData(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    int fontSize = LuaGetArgument_int(L, 2);
    int fontChars = LuaGetArgument_int(L, 3);
    int charsCount = LuaGetArgument_int(L, 4);
    bool sdf = LuaGetArgument_bool(L, 5);
    CharInfo result = LoadFontData(fileName, fontSize, fontChars, charsCount, sdf);
    LuaPush_CharInfo(L, result);
    return 1;
}

// Generate image font atlas using chars info
int lua_GenImageFontAtlas(lua_State *L)
{
    CharInfo chars = LuaGetArgument_CharInfo(L, 1);
    int fontSize = LuaGetArgument_int(L, 2);
    int charsCount = LuaGetArgument_int(L, 3);
    int padding = LuaGetArgument_int(L, 4);
    int packMethod = LuaGetArgument_int(L, 5);
    Image result = GenImageFontAtlas(chars, fontSize, charsCount, padding, packMethod);
    LuaPush_Image(L, result);
    return 1;
}
#endif
// Unload Font from GPU memory (VRAM)
int lua_UnloadFont(lua_State *L)
{
    Font font = LuaGetArgument_Font(L, 1);
    UnloadFont(font);
    return 0;
}

// Text drawing functions
// Shows current FPS
int lua_DrawFPS(lua_State *L)
{
    int posX = LuaGetArgument_int(L, 1);
    int posY = LuaGetArgument_int(L, 2);
    DrawFPS(posX, posY);
    return 0;
}

// Draw text (using default font)
int lua_DrawText(lua_State *L)
{
    const char *text = LuaGetArgument_string(L, 1);
    int posX = LuaGetArgument_int(L, 2);
    int posY = LuaGetArgument_int(L, 3);
    int fontSize = LuaGetArgument_int(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawText(text, posX, posY, fontSize, color);
    return 0;
}

// WARNING: Draw text using font and additional parameters
int lua_DrawTextEx(lua_State *L)
{
    Font font = LuaGetArgument_Font(L, 1);
    const char *text = LuaGetArgument_string(L, 2);
    Vector2 position = LuaGetArgument_Vector2(L, 3);
    float fontSize = LuaGetArgument_float(L, 4);
    float spacing = LuaGetArgument_float(L, 5);
    Color tint = LuaGetArgument_Color(L, 6);
    DrawTextEx(font, text, position, fontSize, spacing, tint);
    return 0;
}

// Text misc. functions
// Measure string width for default font
int lua_MeasureText(lua_State *L)
{
    const char *text = LuaGetArgument_string(L, 1);
    int fontSize = LuaGetArgument_int(L, 2);
    int result = MeasureText(text, fontSize);
    LuaPush_int(L, result);
    return 1;
}

// Measure string size for Font
int lua_MeasureTextEx(lua_State *L)
{
    Font font = LuaGetArgument_Font(L, 1);
    const char *text = LuaGetArgument_string(L, 2);
    float fontSize = LuaGetArgument_float(L, 3);
    float spacing = LuaGetArgument_float(L, 4);
    Vector2 result = MeasureTextEx(font, text, fontSize, spacing);
    LuaPush_Vector2(L, result);
    return 1;
}

// WARNING: FormatText() can be replaced by Lua function: string.format()
// WARNING: SubText() can be replaced by Lua function: string.sub()

// Get index position for a unicode character on font
int lua_GetGlyphIndex(lua_State *L)
{
    Font font = LuaGetArgument_Font(L, 1);
    int character = LuaGetArgument_int(L, 2);
    int result = GetGlyphIndex(font, character);
    LuaPush_int(L, result);
    return 1;
}

//------------------------------------------------------------------------------------
// raylib [models] module functions - Basic 3d Shapes Drawing Functions
//------------------------------------------------------------------------------------

// Draw a line in 3D world space
int lua_DrawLine3D(lua_State *L)
{
    Vector3 startPos = LuaGetArgument_Vector3(L, 1);
    Vector3 endPos = LuaGetArgument_Vector3(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawLine3D(startPos, endPos, color);
    return 0;
}

// Draw a circle in 3D world space
int lua_DrawCircle3D(lua_State *L)
{
    Vector3 center = LuaGetArgument_Vector3(L, 1);
    float radius = LuaGetArgument_float(L, 2);
    Vector3 rotationAxis = LuaGetArgument_Vector3(L, 3);
    float rotationAngle = LuaGetArgument_float(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawCircle3D(center, radius, rotationAxis, rotationAngle, color);
    return 0;
}

// Draw cube
int lua_DrawCube(lua_State *L)
{
    Vector3 position = LuaGetArgument_Vector3(L, 1);
    float width = LuaGetArgument_float(L, 2);
    float height = LuaGetArgument_float(L, 3);
    float length = LuaGetArgument_float(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawCube(position, width, height, length, color);
    return 0;
}

// Draw cube (Vector version)
int lua_DrawCubeV(lua_State *L)
{
    Vector3 position = LuaGetArgument_Vector3(L, 1);
    Vector3 size = LuaGetArgument_Vector3(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawCubeV(position, size, color);
    return 0;
}

// Draw cube wires
int lua_DrawCubeWires(lua_State *L)
{
    Vector3 position = LuaGetArgument_Vector3(L, 1);
    float width = LuaGetArgument_float(L, 2);
    float height = LuaGetArgument_float(L, 3);
    float length = LuaGetArgument_float(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawCubeWires(position, width, height, length, color);
    return 0;
}

#if 0
// Draw cube textured
int lua_DrawCubeTexture(lua_State *L)
{
    Texture2D texture = LuaGetArgument_Texture2D(L, 1);
    Vector3 position = LuaGetArgument_Vector3(L, 2);
    float width = LuaGetArgument_float(L, 3);
    float height = LuaGetArgument_float(L, 4);
    float length = LuaGetArgument_float(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    DrawCubeTexture(texture, position, width, height, length, color);
    return 0;
}
#endif

// Draw sphere
int lua_DrawSphere(lua_State *L)
{
    Vector3 centerPos = LuaGetArgument_Vector3(L, 1);
    float radius = LuaGetArgument_float(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawSphere(centerPos, radius, color);
    return 0;
}

// Draw sphere with extended parameters
int lua_DrawSphereEx(lua_State *L)
{
    Vector3 centerPos = LuaGetArgument_Vector3(L, 1);
    float radius = LuaGetArgument_float(L, 2);
    int rings = LuaGetArgument_int(L, 3);
    int slices = LuaGetArgument_int(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawSphereEx(centerPos, radius, rings, slices, color);
    return 0;
}

// Draw sphere wires
int lua_DrawSphereWires(lua_State *L)
{
    Vector3 centerPos = LuaGetArgument_Vector3(L, 1);
    float radius = LuaGetArgument_float(L, 2);
    int rings = LuaGetArgument_int(L, 3);
    int slices = LuaGetArgument_int(L, 4);
    Color color = LuaGetArgument_Color(L, 5);
    DrawSphereWires(centerPos, radius, rings, slices, color);
    return 0;
}

// Draw a cylinder/cone
int lua_DrawCylinder(lua_State *L)
{
    Vector3 position = LuaGetArgument_Vector3(L, 1);
    float radiusTop = LuaGetArgument_float(L, 2);
    float radiusBottom = LuaGetArgument_float(L, 3);
    float height = LuaGetArgument_float(L, 4);
    int slices = LuaGetArgument_int(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    DrawCylinder(position, radiusTop, radiusBottom, height, slices, color);
    return 0;
}

// Draw a cylinder/cone wires
int lua_DrawCylinderWires(lua_State *L)
{
    Vector3 position = LuaGetArgument_Vector3(L, 1);
    float radiusTop = LuaGetArgument_float(L, 2);
    float radiusBottom = LuaGetArgument_float(L, 3);
    float height = LuaGetArgument_float(L, 4);
    int slices = LuaGetArgument_int(L, 5);
    Color color = LuaGetArgument_Color(L, 6);
    DrawCylinderWires(position, radiusTop, radiusBottom, height, slices, color);
    return 0;
}

// Draw a plane XZ
int lua_DrawPlane(lua_State *L)
{
    Vector3 centerPos = LuaGetArgument_Vector3(L, 1);
    Vector2 size = LuaGetArgument_Vector2(L, 2);
    Color color = LuaGetArgument_Color(L, 3);
    DrawPlane(centerPos, size, color);
    return 0;
}

// Draw a ray line
int lua_DrawRay(lua_State *L)
{
    Ray ray = LuaGetArgument_Ray(L, 1);
    Color color = LuaGetArgument_Color(L, 2);
    DrawRay(ray, color);
    return 0;
}

// Draw a grid (centered at (0, 0, 0))
int lua_DrawGrid(lua_State *L)
{
    int slices = LuaGetArgument_int(L, 1);
    float spacing = LuaGetArgument_float(L, 2);
    DrawGrid(slices, spacing);
    return 0;
}

//------------------------------------------------------------------------------------
// raylib [models] module functions
//------------------------------------------------------------------------------------
#if 0
// Load model from files (mesh and material)
int lua_LoadModel(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Model result = LoadModel(fileName);
    LuaPush_Model(L, result);
    return 1;
}

// Load model from generated mesh
int lua_LoadModelFromMesh(lua_State *L)
{
    Mesh mesh = LuaGetArgument_Mesh(L, 1);
    Model result = LoadModelFromMesh(mesh);
    LuaPush_Model(L, result);
    return 1;
}

// Unload model from memory (RAM and/or VRAM)
int lua_UnloadModel(lua_State *L)
{
    Model model = LuaGetArgument_Model(L, 1);
    UnloadModel(model);
    return 0;
}
#endif

#if 0
// Mesh loading/unloading functions
// Load mesh from file
int lua_LoadMesh(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Mesh result = LoadMesh(fileName);
    LuaPush_Mesh(L, result);
    return 1;
}
#endif

// Unload mesh from memory (RAM and/or VRAM)
int lua_UnloadMesh(lua_State *L)
{
    Mesh mesh = LuaGetArgument_Mesh(L, 1);
    UnloadMesh(mesh);
    return 0;
}

#if 0
// Export mesh as an OBJ file
int lua_ExportMesh(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Mesh mesh = LuaGetArgument_Mesh(L, 2);
    ExportMesh(fileName, mesh);
    return 0;
}
#endif

// Mesh manipulation functions
// Compute mesh bounding box limits
int lua_GetMeshBoundingBox(lua_State *L)
{
    Mesh mesh = LuaGetArgument_Mesh(L, 1);
    BoundingBox result = GetMeshBoundingBox(mesh);
    LuaPush_BoundingBox(L, result);
    return 1;
}

// Compute mesh tangents
int lua_GenMeshTangents(lua_State *L)
{
    Mesh mesh = LuaGetArgument_Mesh(L, 1);
    GenMeshTangents(&mesh);
    return 0;
}

#if 0
// Compute mesh binormals
int lua_MeshBinormals(lua_State *L)
{
    Mesh mesh = LuaGetArgument_Mesh(L, 1);
    MeshBinormals(mesh);
    return 0;
}
#endif

// Mesh generation functions

// Generate plane mesh (with subdivisions)
int lua_GenMeshPlane(lua_State *L)
{
    float width = LuaGetArgument_float(L, 1);
    float length = LuaGetArgument_float(L, 2);
    int resX = LuaGetArgument_int(L, 3);
    int resZ = LuaGetArgument_int(L, 4);
    Mesh result = GenMeshPlane(width, length, resX, resZ);
    LuaPush_Mesh(L, result);
    return 1;
}

// Generate cuboid mesh
int lua_GenMeshCube(lua_State *L)
{
    float width = LuaGetArgument_float(L, 1);
    float height = LuaGetArgument_float(L, 2);
    float length = LuaGetArgument_float(L, 3);
    Mesh result = GenMeshCube(width, height, length);
    LuaPush_Mesh(L, result);
    return 1;
}

// Generate sphere mesh (standard sphere)
int lua_GenMeshSphere(lua_State *L)
{
    float radius = LuaGetArgument_float(L, 1);
    int rings = LuaGetArgument_int(L, 2);
    int slices = LuaGetArgument_int(L, 3);
    Mesh result = GenMeshSphere(radius, rings, slices);
    LuaPush_Mesh(L, result);
    return 1;
}

// Generate half-sphere mesh (no bottom cap)
int lua_GenMeshHemiSphere(lua_State *L)
{
    float radius = LuaGetArgument_float(L, 1);
    int rings = LuaGetArgument_int(L, 2);
    int slices = LuaGetArgument_int(L, 3);
    Mesh result = GenMeshHemiSphere(radius, rings, slices);
    LuaPush_Mesh(L, result);
    return 1;
}

// Generate cylinder mesh
int lua_GenMeshCylinder(lua_State *L)
{
    float radius = LuaGetArgument_float(L, 1);
    float height = LuaGetArgument_float(L, 2);
    int slices = LuaGetArgument_int(L, 3);
    Mesh result = GenMeshCylinder(radius, height, slices);
    LuaPush_Mesh(L, result);
    return 1;
}

// Generate torus mesh
int lua_GenMeshTorus(lua_State *L)
{
    float radius = LuaGetArgument_float(L, 1);
    float size = LuaGetArgument_float(L, 2);
    int radSeg = LuaGetArgument_int(L, 3);
    int sides = LuaGetArgument_int(L, 4);
    Mesh result = GenMeshTorus(radius, size, radSeg, sides);
    LuaPush_Mesh(L, result);
    return 1;
}

// Generate trefoil knot mesh
int lua_GenMeshKnot(lua_State *L)
{
    float radius = LuaGetArgument_float(L, 1);
    float size = LuaGetArgument_float(L, 2);
    int radSeg = LuaGetArgument_int(L, 3);
    int sides = LuaGetArgument_int(L, 4);
    Mesh result = GenMeshKnot(radius, size, radSeg, sides);
    LuaPush_Mesh(L, result);
    return 1;
}

// Generate heightmap mesh from image data
int lua_GenMeshHeightmap(lua_State *L)
{
    Image heightmap = LuaGetArgument_Image(L, 1);
    Vector3 size = LuaGetArgument_Vector3(L, 2);
    Mesh result = GenMeshHeightmap(heightmap, size);
    LuaPush_Mesh(L, result);
    return 1;
}

// Generate cubes-based map mesh from image data
int lua_GenMeshCubicmap(lua_State *L)
{
    Image cubicmap = LuaGetArgument_Image(L, 1);
    Vector3 cubeSize = LuaGetArgument_Vector3(L, 2);
    Mesh result = GenMeshCubicmap(cubicmap, cubeSize);
    LuaPush_Mesh(L, result);
    return 1;
}

#if 0
// Material loading/unloading functions
// Load material from file
int lua_LoadMaterial(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Material result = LoadMaterial(fileName);
    LuaPush_Material(L, result);
    return 1;
}


// Load default material (Supports: DIFFUSE, SPECULAR, NORMAL maps)
int lua_LoadMaterialDefault(lua_State *L)
{
    Material result = LoadMaterialDefault();
    LuaPush_Material(L, result);
    return 1;
}

// Unload material from GPU memory (VRAM)
int lua_UnloadMaterial(lua_State *L)
{
    Material material = LuaGetArgument_Material(L, 1);
    UnloadMaterial(material);
    return 0;
}
#endif

#if 0
// Model drawing functions
// Draw a model (with texture if set)
int lua_DrawModel(lua_State *L)
{
    Model model = LuaGetArgument_Model(L, 1);
    Vector3 position = LuaGetArgument_Vector3(L, 2);
    float scale = LuaGetArgument_float(L, 3);
    Color tint = LuaGetArgument_Color(L, 4);
    DrawModel(model, position, scale, tint);
    return 0;
}

// Draw a model with extended parameters
int lua_DrawModelEx(lua_State *L)
{
    Model model = LuaGetArgument_Model(L, 1);
    Vector3 position = LuaGetArgument_Vector3(L, 2);
    Vector3 rotationAxis = LuaGetArgument_Vector3(L, 3);
    float rotationAngle = LuaGetArgument_float(L, 4);
    Vector3 scale = LuaGetArgument_Vector3(L, 5);
    Color tint = LuaGetArgument_Color(L, 6);
    DrawModelEx(model, position, rotationAxis, rotationAngle, scale, tint);
    return 0;
}

// Draw a model wires (with texture if set)
int lua_DrawModelWires(lua_State *L)
{
    Model model = LuaGetArgument_Model(L, 1);
    Vector3 position = LuaGetArgument_Vector3(L, 2);
    float scale = LuaGetArgument_float(L, 3);
    Color tint = LuaGetArgument_Color(L, 4);
    DrawModelWires(model, position, scale, tint);
    return 0;
}

// Draw a model wires (with texture if set) with extended parameters
int lua_DrawModelWiresEx(lua_State *L)
{
    Model model = LuaGetArgument_Model(L, 1);
    Vector3 position = LuaGetArgument_Vector3(L, 2);
    Vector3 rotationAxis = LuaGetArgument_Vector3(L, 3);
    float rotationAngle = LuaGetArgument_float(L, 4);
    Vector3 scale = LuaGetArgument_Vector3(L, 5);
    Color tint = LuaGetArgument_Color(L, 6);
    DrawModelWiresEx(model, position, rotationAxis, rotationAngle, scale, tint);
    return 0;
}
#endif

// Draw bounding box (wires)
int lua_DrawBoundingBox(lua_State *L)
{
    BoundingBox box = LuaGetArgument_BoundingBox(L, 1);
    Color color = LuaGetArgument_Color(L, 2);
    DrawBoundingBox(box, color);
    return 0;
}

// Draw a billboard texture
int lua_DrawBillboard(lua_State *L)
{
    Camera camera = LuaGetArgument_Camera(L, 1);
    Texture2D texture = LuaGetArgument_Texture2D(L, 2);
    Vector3 center = LuaGetArgument_Vector3(L, 3);
    float size = LuaGetArgument_float(L, 4);
    Color tint = LuaGetArgument_Color(L, 5);
    DrawBillboard(camera, texture, center, size, tint);
    return 0;
}

// Draw a billboard texture defined by sourceRec
int lua_DrawBillboardRec(lua_State *L)
{
    Camera camera = LuaGetArgument_Camera(L, 1);
    Texture2D texture = LuaGetArgument_Texture2D(L, 2);
    Rectangle sourceRec = LuaGetArgument_Rectangle(L, 3);
    Vector3 center = LuaGetArgument_Vector3(L, 4);
    Vector2 size = LuaGetArgument_Vector2(L, 5);
    Color tint = LuaGetArgument_Color(L, 6);
    DrawBillboardRec(camera, texture, sourceRec, center, size, tint);
    return 0;
}

// Collision detection functions
// Detect collision between two spheres
int lua_CheckCollisionSpheres(lua_State *L)
{
    Vector3 centerA = LuaGetArgument_Vector3(L, 1);
    float radiusA = LuaGetArgument_float(L, 2);
    Vector3 centerB = LuaGetArgument_Vector3(L, 3);
    float radiusB = LuaGetArgument_float(L, 4);
    bool result = CheckCollisionSpheres(centerA, radiusA, centerB, radiusB);
    LuaPush_bool(L, result);
    return 1;
}

// Detect collision between two bounding boxes
int lua_CheckCollisionBoxes(lua_State *L)
{
    BoundingBox box1 = LuaGetArgument_BoundingBox(L, 1);
    BoundingBox box2 = LuaGetArgument_BoundingBox(L, 2);
    bool result = CheckCollisionBoxes(box1, box2);
    LuaPush_bool(L, result);
    return 1;
}

// Detect collision between box and sphere
int lua_CheckCollisionBoxSphere(lua_State *L)
{
    BoundingBox box = LuaGetArgument_BoundingBox(L, 1);
    Vector3 centerSphere = LuaGetArgument_Vector3(L, 2);
    float radiusSphere = LuaGetArgument_float(L, 3);
    bool result = CheckCollisionBoxSphere(box, centerSphere, radiusSphere);
    LuaPush_bool(L, result);
    return 1;
}

#if 0
// Detect collision between ray and sphere
int lua_CheckCollisionRaySphere(lua_State *L)
{
    Ray ray = LuaGetArgument_Ray(L, 1);
    Vector3 spherePosition = LuaGetArgument_Vector3(L, 2);
    float sphereRadius = LuaGetArgument_float(L, 3);
    bool result = CheckCollisionRaySphere(ray, spherePosition, sphereRadius);
    LuaPush_bool(L, result);
    return 1;
}

// Detect collision between ray and sphere, returns collision point
int lua_CheckCollisionRaySphereEx(lua_State *L)
{
    Ray ray = LuaGetArgument_Ray(L, 1);
    Vector3 spherePosition = LuaGetArgument_Vector3(L, 2);
    float sphereRadius = LuaGetArgument_float(L, 3);
    Vector3 collisionPoint = LuaGetArgument_Vector3(L, 4);
    bool result = CheckCollisionRaySphereEx(ray, spherePosition, sphereRadius, collisionPoint);
    LuaPush_bool(L, result);
    return 1;
}

// Detect collision between ray and box
int lua_CheckCollisionRayBox(lua_State *L)
{
    Ray ray = LuaGetArgument_Ray(L, 1);
    BoundingBox box = LuaGetArgument_BoundingBox(L, 2);
    bool result = CheckCollisionRayBox(ray, box);
    LuaPush_bool(L, result);
    return 1;
}
#endif

//------------------------------------------------------------------------------------
// raylib [raymath] module functions - Shaders
//------------------------------------------------------------------------------------

// Load shader from files and bind default locations
int lua_LoadShader(lua_State *L)
{
    const char *vsFileName = LuaGetArgument_string(L, 1);
    const char *fsFileName = LuaGetArgument_string(L, 2);
    Shader result = LoadShader(vsFileName, fsFileName);
    LuaPush_Shader(L, result);
    return 1;
}

// Load shader from code strings and bind default locations
int lua_LoadShaderFromMemory(lua_State *L)
{
    const char *vsCode = LuaGetArgument_string(L, 1);
    const char *fsCode = LuaGetArgument_string(L, 2);
    Shader result = LoadShaderFromMemory(vsCode, fsCode);
    LuaPush_Shader(L, result);
    return 1;
}

// Unload shader from GPU memory (VRAM)
int lua_UnloadShader(lua_State *L)
{
    Shader shader = LuaGetArgument_Shader(L, 1);
    UnloadShader(shader);
    return 0;
}

#if 0
// Get default shader
int lua_GetShaderDefault(lua_State *L)
{
    Shader result = GetShaderDefault();
    LuaPush_Shader(L, result);
    return 1;
}

// Get default texture
int lua_GetTextureDefault(lua_State *L)
{
    Texture2D result = GetTextureDefault();
    LuaPush_Texture2D(L, result);
    return 1;
}
#endif

// Shader configuration functions

// Get shader uniform location
int lua_GetShaderLocation(lua_State *L)
{
    Shader shader = LuaGetArgument_Shader(L, 1);
    const char *uniformName = LuaGetArgument_string(L, 2);
    int result = GetShaderLocation(shader, uniformName);
    LuaPush_int(L, result);
    return 1;
}

// WARNING: Set shader uniform values (float)
int lua_SetShaderValue(lua_State* L)
{
    Shader arg1 = LuaGetArgument_Shader(L, 1);
    int arg2 = LuaGetArgument_int(L, 2);
    GET_TABLE(float, arg3, 3);
    SetShaderValue(arg1, arg2, arg3, arg3_size);
    free(arg3);
    return 0;
}

#if 0
// WARNING: Set shader uniform values (int)
int lua_SetShaderValuei(lua_State* L)
{
    Shader arg1 = LuaGetArgument_Shader(L, 1);
    int arg2 = LuaGetArgument_int(L, 2);
    GET_TABLE(int, arg3, 3);
    SetShaderValuei(arg1, arg2, arg3, arg3_size);
    free(arg3);
    return 0;
}
#endif

// Set shader uniform value (matrix 4x4)
int lua_SetShaderValueMatrix(lua_State *L)
{
    Shader shader = LuaGetArgument_Shader(L, 1);
    int uniformLoc = LuaGetArgument_int(L, 2);
    Matrix mat = LuaGetArgument_Matrix(L, 3);
    SetShaderValueMatrix(shader, uniformLoc, mat);
    return 0;
}

#if 0
// Set a custom projection matrix (replaces internal projection matrix)
int lua_SetMatrixProjection(lua_State *L)
{
    Matrix proj = LuaGetArgument_Matrix(L, 1);
    SetMatrixProjection(proj);
    return 0;
}

// Set a custom modelview matrix (replaces internal modelview matrix)
int lua_SetMatrixModelview(lua_State *L)
{
    Matrix view = LuaGetArgument_Matrix(L, 1);
    SetMatrixModelview(view);
    return 0;
}

// Get internal modelview matrix
int lua_GetMatrixModelview(lua_State *L)
{
    Matrix view = LuaGetArgument_Matrix(L, 1);
    Matrix result = GetMatrixModelview(view);
    LuaPush_Matrix(L, &result);
    return 1;
}

// Texture maps generation (PBR)
// NOTE: Required shaders should be provided
// Generate cubemap texture from HDR texture
int lua_GenTextureCubemap(lua_State *L)
{
    Shader shader = LuaGetArgument_Shader(L, 1);
    Texture2D skyHDR = LuaGetArgument_Texture2D(L, 2);
    int size = LuaGetArgument_int(L, 3);
    Texture2D result = GenTextureCubemap(shader, skyHDR, size);
    LuaPush_Texture2D(L, result);
    return 1;
}

// Generate irradiance texture using cubemap data
int lua_GenTextureIrradiance(lua_State *L)
{
    Shader shader = LuaGetArgument_Shader(L, 1);
    Texture2D cubemap = LuaGetArgument_Texture2D(L, 2);
    int size = LuaGetArgument_int(L, 3);
    Texture2D result = GenTextureIrradiance(shader, cubemap, size);
    LuaPush_Texture2D(L, result);
    return 1;
}

// Generate prefilter texture using cubemap data
int lua_GenTexturePrefilter(lua_State *L)
{
    Shader shader = LuaGetArgument_Shader(L, 1);
    Texture2D cubemap = LuaGetArgument_Texture2D(L, 2);
    int size = LuaGetArgument_int(L, 3);
    Texture2D result = GenTexturePrefilter(shader, cubemap, size);
    LuaPush_Texture2D(L, result);
    return 1;
}

// Generate BRDF texture using cubemap data
int lua_GenTextureBRDF(lua_State *L)
{
    Shader shader = LuaGetArgument_Shader(L, 1);
    Texture2D cubemap = LuaGetArgument_Texture2D(L, 2);
    int size = LuaGetArgument_int(L, 3);
    Texture2D result = GenTextureBRDF(shader, cubemap, size);
    LuaPush_Texture2D(L, result);
    return 1;
}
#endif

// Shading begin/end functions
// Begin custom shader drawing
int lua_BeginShaderMode(lua_State *L)
{
    Shader shader = LuaGetArgument_Shader(L, 1);
    BeginShaderMode(shader);
    return 0;
}

// End custom shader drawing (use default shader)
int lua_EndShaderMode(lua_State *L)
{
    EndShaderMode();
    return 0;
}

// Begin blending mode (alpha, additive, multiplied)
int lua_BeginBlendMode(lua_State *L)
{
    int mode = LuaGetArgument_int(L, 1);
    BeginBlendMode(mode);
    return 0;
}

// End blending mode (reset to default: alpha blending)
int lua_EndBlendMode(lua_State *L)
{
    EndBlendMode();
    return 0;
}

//------------------------------------------------------------------------------------
// raylib [rlgl] module functions - VR experience
//------------------------------------------------------------------------------------

#if 0

// Get VR device information for some standard devices
int lua_GetVrDeviceInfo(lua_State *L)
{
    int vrDeviceType = LuaGetArgument_int(L, 1);
    VrDeviceInfo result = GetVrDeviceInfo(vrDeviceType);
    LuaPush_VrDeviceInfo(L, result);
    return 1;
}

// Init VR simulator for selected device parameters
int lua_InitVrSimulator(lua_State *L)
{
    VrDeviceInfo info = LuaGetArgument_VrDeviceInfo(L, 1);
    InitVrSimulator(info);
    return 0;
}

// Close VR simulator for current device
int lua_CloseVrSimulator(lua_State *L)
{
    CloseVrSimulator();
    return 0;
}

// Detect if VR simulator is ready
int lua_IsVrSimulatorReady(lua_State *L)
{
    bool result = IsVrSimulatorReady();
    LuaPush_bool(L, result);
    return 1;
}

// Set VR distortion shader for stereoscopic rendering
int lua_SetVrDistortionShader(lua_State *L)
{
    Shader shader = LuaGetArgument_Shader(L, 1);
    SetVrDistortionShader(shader);
    return 0;
}

// Update VR tracking (position and orientation) and camera
int lua_UpdateVrTracking(lua_State *L)
{
    Camera camera = LuaGetArgument_Camera(L, 1);
    UpdateVrTracking(camera);
    return 0;
}

// Enable/Disable VR experience
int lua_ToggleVrMode(lua_State *L)
{
    ToggleVrMode();
    return 0;
}

// Begin VR simulator stereo rendering
int lua_BeginVrDrawing(lua_State *L)
{
    BeginVrDrawing();
    return 0;
}

// End VR simulator stereo rendering
int lua_EndVrDrawing(lua_State *L)
{
    EndVrDrawing();
    return 0;
}
#endif

//------------------------------------------------------------------------------------
// raylib [audio] module functions - Audio Loading and Playing
//------------------------------------------------------------------------------------

// Initialize audio device and context
int lua_InitAudioDevice(lua_State *L)
{
    InitAudioDevice();
    return 0;
}

// Close the audio device and context
int lua_CloseAudioDevice(lua_State *L)
{
    CloseAudioDevice();
    return 0;
}

// Check if audio device has been initialized successfully
int lua_IsAudioDeviceReady(lua_State *L)
{
    bool result = IsAudioDeviceReady();
    LuaPush_bool(L, result);
    return 1;
}

// Set master volume (listener)
int lua_SetMasterVolume(lua_State *L)
{
    float volume = LuaGetArgument_float(L, 1);
    SetMasterVolume(volume);
    return 0;
}

// Wave/Sound loading/unloading functions
// Load wave data from file
int lua_LoadWave(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Wave result = LoadWave(fileName);
    LuaPush_Wave(L, result);
    return 1;
}

#if 0
// WARNING: Load wave data from raw array data
int lua_LoadWaveEx(lua_State* L)
{
    // TODO: arg1 parameter should be a float arrat...
    
    float *arg1 = 0;
    int arg2 = LuaGetArgument_int(L, 2);
    int arg3 = LuaGetArgument_int(L, 3);
    int arg4 = LuaGetArgument_int(L, 4);
    int arg5 = LuaGetArgument_int(L, 5);
    Wave result = LoadWaveEx(arg1, arg2, arg3, arg4, arg5);
    LuaPush_Wave(L, result);
    return 1;
}
#endif

// Load sound from file
int lua_LoadSound(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Sound result = LoadSound(fileName);
    LuaPush_Sound(L, result);
    return 1;
}

// Load sound from wave data
int lua_LoadSoundFromWave(lua_State *L)
{
    Wave wave = LuaGetArgument_Wave(L, 1);
    Sound result = LoadSoundFromWave(wave);
    LuaPush_Sound(L, result);
    return 1;
}

// WARNING: Update sound buffer with new data
int lua_UpdateSound(lua_State* L)
{
    // TODO: arg2 parameter is a void pointer...
    
    Sound arg1 = LuaGetArgument_Sound(L, 1);
    void *arg2 = LuaGetArgument_ptr(L, 2);
    int arg3 = LuaGetArgument_int(L, 3);
    UpdateSound(arg1, arg2, arg3);
    return 0;
}

// Unload wave data
int lua_UnloadWave(lua_State *L)
{
    Wave wave = LuaGetArgument_Wave(L, 1);
    UnloadWave(wave);
    return 0;
}

// Unload sound
int lua_UnloadSound(lua_State *L)
{
    Sound sound = LuaGetArgument_Sound(L, 1);
    UnloadSound(sound);
    return 0;
}

// Wave/Sound management functions
// Play a sound
int lua_PlaySound(lua_State *L)
{
    Sound sound = LuaGetArgument_Sound(L, 1);
    PlaySound(sound);
    return 0;
}

// Pause a sound
int lua_PauseSound(lua_State *L)
{
    Sound sound = LuaGetArgument_Sound(L, 1);
    PauseSound(sound);
    return 0;
}

// Resume a paused sound
int lua_ResumeSound(lua_State *L)
{
    Sound sound = LuaGetArgument_Sound(L, 1);
    ResumeSound(sound);
    return 0;
}

// Stop playing a sound
int lua_StopSound(lua_State *L)
{
    Sound sound = LuaGetArgument_Sound(L, 1);
    StopSound(sound);
    return 0;
}

// Check if a sound is currently playing
int lua_IsSoundPlaying(lua_State *L)
{
    Sound sound = LuaGetArgument_Sound(L, 1);
    bool result = IsSoundPlaying(sound);
    LuaPush_bool(L, result);
    return 1;
}

// Set volume for a sound (1.0 is max level)
int lua_SetSoundVolume(lua_State *L)
{
    Sound sound = LuaGetArgument_Sound(L, 1);
    float volume = LuaGetArgument_float(L, 2);
    SetSoundVolume(sound, volume);
    return 0;
}

// Set pitch for a sound (1.0 is base level)
int lua_SetSoundPitch(lua_State *L)
{
    Sound sound = LuaGetArgument_Sound(L, 1);
    float pitch = LuaGetArgument_float(L, 2);
    SetSoundPitch(sound, pitch);
    return 0;
}

// Convert wave data to desired format
int lua_WaveFormat(lua_State *L)
{
    Wave wave = LuaGetArgument_Wave(L, 1);
    int sampleRate = LuaGetArgument_int(L, 2);
    int sampleSize = LuaGetArgument_int(L, 3);
    int channels = LuaGetArgument_int(L, 4);
    WaveFormat(&wave, sampleRate, sampleSize, channels);
    return 0;
}

// Copy a wave to a new wave
int lua_WaveCopy(lua_State *L)
{
    Wave wave = LuaGetArgument_Wave(L, 1);
    Wave result = WaveCopy(wave);
    LuaPush_Wave(L, result);
    return 1;
}

// Crop a wave to defined samples range
int lua_WaveCrop(lua_State *L)
{
    Wave wave = LuaGetArgument_Wave(L, 1);
    int initSample = LuaGetArgument_int(L, 2);
    int finalSample = LuaGetArgument_int(L, 3);
    WaveCrop(&wave, initSample, finalSample);
    return 0;
}

#if 0
// WARNING: Get samples data from wave as a floats array
int lua_GetWaveData(lua_State* L)
{
    // TODO: return value should be a float array...
    
    Wave arg1 = LuaGetArgument_Wave(L, 1);
    float *result = GetWaveData(arg1);
    //LuaPush_float(L, result);
    //LuaPush_float(L, result);
    return 0;
}
#endif

// Music management functions

// Load music stream from file
int lua_LoadMusicStream(lua_State *L)
{
    const char *fileName = LuaGetArgument_string(L, 1);
    Music result = LoadMusicStream(fileName);
    LuaPush_Music(L, result);
    return 1;
}

// Unload music stream
int lua_UnloadMusicStream(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    UnloadMusicStream(music);
    return 0;
}

// Start music playing
int lua_PlayMusicStream(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    PlayMusicStream(music);
    return 0;
}

// Updates buffers for music streaming
int lua_UpdateMusicStream(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    UpdateMusicStream(music);
    return 0;
}

// Stop music playing
int lua_StopMusicStream(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    StopMusicStream(music);
    return 0;
}

// Pause music playing
int lua_PauseMusicStream(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    PauseMusicStream(music);
    return 0;
}

// Resume playing paused music
int lua_ResumeMusicStream(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    ResumeMusicStream(music);
    return 0;
}

#if 0
// Check if music is playing
int lua_IsMusicPlaying(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    bool result = IsMusicPlaying(music);
    LuaPush_bool(L, result);
    return 1;
}
#endif

// Set volume for music (1.0 is max level)
int lua_SetMusicVolume(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    float volume = LuaGetArgument_float(L, 2);
    SetMusicVolume(music, volume);
    return 0;
}

// Set pitch for a music (1.0 is base level)
int lua_SetMusicPitch(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    float pitch = LuaGetArgument_float(L, 2);
    SetMusicPitch(music, pitch);
    return 0;
}

#if 0
// Set music loop count (loop repeats)
int lua_SetMusicLoopCount(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    int count = LuaGetArgument_int(L, 2);
    SetMusicLoopCount(music, count);
    return 0;
}
#endif

// Get music time length (in seconds)
int lua_GetMusicTimeLength(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    float result = GetMusicTimeLength(music);
    LuaPush_float(L, result);
    return 1;
}

// Get current music time played (in seconds)
int lua_GetMusicTimePlayed(lua_State *L)
{
    Music music = LuaGetArgument_Music(L, 1);
    float result = GetMusicTimePlayed(music);
    LuaPush_float(L, result);
    return 1;
}

#if 0
// AudioStream management functions
// Init audio stream (to stream raw audio pcm data)
int lua_InitAudioStream(lua_State *L)
{
    unsigned int sampleRate = LuaGetArgument_unsigned(L, 1);
    unsigned int sampleSize = LuaGetArgument_unsigned(L, 2);
    unsigned int channels = LuaGetArgument_unsigned(L, 3);
    AudioStream result = InitAudioStream(sampleRate, sampleSize, channels);
    LuaPush_AudioStream(L, result);
    return 1;
}

// WARNING: Update audio stream buffers with data
int lua_UpdateAudioStream(lua_State* L)
{
    // TODO: arg2 parameter is a void pointer...
    
    AudioStream arg1 = LuaGetArgument_AudioStream(L, 1);
    void *arg2 = LuaGetArgument_ptr(L, 2);
    int arg3 = LuaGetArgument_int(L, 3);
    UpdateAudioStream(arg1, arg2, arg3);
    return 0;
}

// Close audio stream and free memory
int lua_CloseAudioStream(lua_State *L)
{
    AudioStream stream = LuaGetArgument_AudioStream(L, 1);
    CloseAudioStream(stream);
    return 0;
}

// Check if any audio stream buffers requires refill
int lua_IsAudioBufferProcessed(lua_State *L)
{
    AudioStream stream = LuaGetArgument_AudioStream(L, 1);
    bool result = IsAudioBufferProcessed(stream);
    LuaPush_bool(L, result);
    return 1;
}
#endif

// Play audio stream
int lua_PlayAudioStream(lua_State *L)
{
    AudioStream stream = LuaGetArgument_AudioStream(L, 1);
    PlayAudioStream(stream);
    return 0;
}

// Pause audio stream
int lua_PauseAudioStream(lua_State *L)
{
    AudioStream stream = LuaGetArgument_AudioStream(L, 1);
    PauseAudioStream(stream);
    return 0;
}

// Resume audio stream
int lua_ResumeAudioStream(lua_State *L)
{
    AudioStream stream = LuaGetArgument_AudioStream(L, 1);
    ResumeAudioStream(stream);
    return 0;
}

// Check if audio stream is playing
int lua_IsAudioStreamPlaying(lua_State *L)
{
    AudioStream stream = LuaGetArgument_AudioStream(L, 1);
    bool result = IsAudioStreamPlaying(stream);
    LuaPush_bool(L, result);
    return 1;
}

// Stop audio stream
int lua_StopAudioStream(lua_State *L)
{
    AudioStream stream = LuaGetArgument_AudioStream(L, 1);
    StopAudioStream(stream);
    return 0;
}

// Set volume for audio stream (1.0 is max level)
int lua_SetAudioStreamVolume(lua_State *L)
{
    AudioStream stream = LuaGetArgument_AudioStream(L, 1);
    float volume = LuaGetArgument_float(L, 2);
    SetAudioStreamVolume(stream, volume);
    return 0;
}

// Set pitch for audio stream (1.0 is base level)
int lua_SetAudioStreamPitch(lua_State *L)
{
    AudioStream stream = LuaGetArgument_AudioStream(L, 1);
    float pitch = LuaGetArgument_float(L, 2);
    SetAudioStreamPitch(stream, pitch);
    return 0;
}

//----------------------------------------------------------------------------------
// Functions Registering
//----------------------------------------------------------------------------------
#define REG(name) { #name, lua_##name },

static luaL_Reg raylib_functions[] = {

    // Register non-opaque data types
    REG(Color)
    REG(Vector2)
    REG(Vector3)
    REG(Vector4)
    //REG(Matrix)
    REG(Quaternion)
    REG(Rectangle)
    REG(Ray)
    //REG(RayHitInfo)
    REG(Camera)
    REG(Camera2D)
    REG(BoundingBox)
    //REG(Material)

    // Register functions
    //--------------------
    REG(InitWindow)
    REG(CloseWindow)
    REG(IsWindowReady)
    REG(WindowShouldClose)
    REG(IsWindowMinimized)
    REG(ToggleFullscreen)
    REG(SetWindowIcon)
    REG(SetWindowTitle)
    REG(SetWindowPosition)
    REG(SetWindowMonitor)
    REG(SetWindowMinSize)
    REG(SetWindowSize)
    REG(GetScreenWidth)
    REG(GetScreenHeight)
    REG(ShowCursor)
    REG(HideCursor)
    REG(IsCursorHidden)
    REG(EnableCursor)
    REG(DisableCursor)
    REG(ClearBackground)
    REG(BeginDrawing)
    REG(EndDrawing)
    REG(BeginMode2D)
    REG(EndMode2D)
    REG(BeginMode3D)
    REG(EndMode3D)
    REG(BeginTextureMode)
    REG(EndTextureMode)
    REG(GetMouseRay)
    REG(GetWorldToScreen)
    REG(GetCameraMatrix)
    REG(SetTargetFPS)
    REG(GetFPS)
    REG(GetFrameTime)
    REG(GetTime)
    REG(ColorToInt)
    REG(ColorNormalize)
    REG(ColorToHSV)
    REG(GetColor)
    REG(Fade)
    //REG(ShowLogo)
    //REG(SetConfigFlags)
    REG(SetTraceLogLevel)
    //REG(TraceLog)
    REG(TakeScreenshot)
    REG(GetRandomValue)
    REG(IsFileExtension)
    //REG(GetExtension)
    REG(GetFileName)
    REG(GetDirectoryPath)
    REG(GetWorkingDirectory)
    REG(ChangeDirectory)
    REG(IsFileDropped)
    //REG(GetDroppedFiles)
    //REG(ClearDroppedFiles)
    //REG(StorageSaveValue)
    //REG(StorageLoadValue)
    REG(IsKeyPressed)
    REG(IsKeyDown)
    REG(IsKeyReleased)
    REG(IsKeyUp)
    REG(GetKeyPressed)
    REG(GetCharPressed)
    REG(SetExitKey)
    REG(IsGamepadAvailable)
    //REG(IsGamepadName)
    REG(GetGamepadName)
    REG(IsGamepadButtonPressed)
    REG(IsGamepadButtonDown)
    REG(IsGamepadButtonReleased)
    REG(IsGamepadButtonUp)
    REG(GetGamepadButtonPressed)
    REG(GetGamepadAxisCount)
    REG(GetGamepadAxisMovement)
    REG(IsMouseButtonPressed)
    REG(IsMouseButtonDown)
    REG(IsMouseButtonReleased)
    REG(IsMouseButtonUp)
    REG(GetMouseX)
    REG(GetMouseY)
    REG(GetMousePosition)
    REG(SetMousePosition)
    REG(SetMouseScale)
    REG(GetMouseWheelMove)
    REG(GetTouchX)
    REG(GetTouchY)
    REG(GetTouchPosition)
    REG(SetGesturesEnabled)
    REG(IsGestureDetected)
    REG(GetGestureDetected)
    REG(GetTouchPointCount)
    REG(GetGestureHoldDuration)
    REG(GetGestureDragVector)
    REG(GetGestureDragAngle)
    REG(GetGesturePinchVector)
    REG(GetGesturePinchAngle)
    REG(UpdateCamera)
    REG(DrawPixel)
    REG(DrawPixelV)
    REG(DrawLine)
    REG(DrawLineV)
    REG(DrawLineEx)
    REG(DrawLineStrip)
    REG(DrawLineBezier)
    REG(DrawCircle)
    REG(DrawCircleSector)
    REG(DrawCircleSectorLines)
    REG(DrawCircleGradient)
    REG(DrawCircleV)
    REG(DrawCircleLines)
    REG(DrawCircleLinesV)
    REG(DrawEllipse)
    REG(DrawEllipseLines)
    REG(DrawRing)
    REG(DrawRingLines)
    REG(DrawRectangle)
    REG(DrawRectangleV)
    REG(DrawRectangleRec)
    REG(DrawRectanglePro)
    REG(DrawRectangleGradientV)
    REG(DrawRectangleGradientH)
    REG(DrawRectangleGradientEx)
    REG(DrawRectangleLines)
    REG(DrawRectangleLinesEx)
    REG(DrawRectangleRounded)
    REG(DrawRectangleRoundedLines)
    REG(DrawTriangle)
    REG(DrawTriangleLines)
    REG(DrawTriangleFan)
    REG(DrawTriangleStrip)
    REG(DrawPoly)
    REG(DrawPolyLines)
    REG(DrawPolyLinesEx)
    REG(DrawSplineLinear)
    REG(DrawSplineBasis)
    REG(DrawSplineCatmullRom)
    REG(DrawSplineBezierQuadratic)
    REG(DrawSplineBezierCubic)
    REG(DrawSplineSegmentLinear)
    REG(DrawSplineSegmentBasis)
    REG(DrawSplineSegmentCatmullRom)
    REG(DrawSplineSegmentBezierQuadratic)
    REG(DrawSplineSegmentBezierCubic)
    
    REG(CheckCollisionRecs)
    REG(CheckCollisionCircles)
    REG(CheckCollisionCircleRec)
    REG(GetCollisionRec)
    REG(CheckCollisionPointRec)
    REG(CheckCollisionPointCircle)
    REG(CheckCollisionPointTriangle)
    REG(LoadImage)
    //REG(LoadImageEx)
    //REG(LoadImagePro)
    REG(LoadImageRaw)
    REG(ExportImage)
    REG(LoadTexture)
    REG(LoadTextureFromImage)
    REG(LoadRenderTexture)
    REG(UnloadImage)
    REG(UnloadTexture)
    REG(UnloadRenderTexture)
    //REG(GetImageData)
    //REG(GetImageDataNormalized)
    REG(GetPixelDataSize)
    //REG(GetTextureData)
    REG(UpdateTexture)
    REG(ImageCopy)
    REG(ImageToPOT)
    REG(ImageFormat)
    REG(ImageAlphaMask)
    REG(ImageAlphaClear)
    REG(ImageAlphaCrop)
    REG(ImageAlphaPremultiply)
    REG(ImageCrop)
    REG(ImageResize)
    REG(ImageResizeNN)
    REG(ImageResizeCanvas)
    REG(ImageMipmaps)
    REG(ImageDither)
    REG(ImageText)
    REG(ImageTextEx)
    REG(ImageDraw)
    REG(ImageDrawRectangle)
    REG(ImageDrawText)
    REG(ImageDrawTextEx)
    REG(ImageFlipVertical)
    REG(ImageFlipHorizontal)
    REG(ImageRotateCW)
    REG(ImageRotateCCW)
    REG(ImageColorTint)
    REG(ImageColorInvert)
    REG(ImageColorGrayscale)
    REG(ImageColorContrast)
    REG(ImageColorBrightness)
    REG(ImageColorReplace)
    REG(GenImageColor)
    //REG(GenImageGradientV)
    //REG(GenImageGradientH)
    REG(GenImageGradientRadial)
    REG(GenImageChecked)
    REG(GenImageWhiteNoise)
    REG(GenImagePerlinNoise)
    REG(GenImageCellular)
    REG(GenTextureMipmaps)
    REG(SetTextureFilter)
    REG(SetTextureWrap)
    REG(DrawTexture)
    REG(DrawTextureV)
    REG(DrawTextureEx)
    REG(DrawTextureRec)
    REG(DrawTexturePro)
    REG(GetFontDefault)
    REG(LoadFont)
    //REG(LoadFontEx)
    //REG(LoadFontData)
    //REG(GenImageFontAtlas)
    REG(UnloadFont)
    REG(DrawFPS)
    REG(DrawText)
    REG(DrawTextEx)
    REG(MeasureText)
    REG(MeasureTextEx)
    //REG(FormatText)
    //REG(SubText)
    REG(GetGlyphIndex)
    REG(DrawLine3D)
    REG(DrawCircle3D)
    REG(DrawCube)
    REG(DrawCubeV)
    REG(DrawCubeWires)
    //REG(DrawCubeTexture)
    REG(DrawSphere)
    REG(DrawSphereEx)
    REG(DrawSphereWires)
    REG(DrawCylinder)
    REG(DrawCylinderWires)
    REG(DrawPlane)
    REG(DrawRay)
    REG(DrawGrid)
    
    //REG(LoadModel)
    //REG(LoadModelFromMesh)
    //REG(UnloadModel)
    //REG(LoadMesh)
    REG(UnloadMesh)
    //REG(ExportMesh)
    //REG(MeshBoundingBox)
    //REG(MeshTangents)
    //REG(MeshBinormals)
    REG(GenMeshPlane)
    REG(GenMeshCube)
    REG(GenMeshSphere)
    REG(GenMeshHemiSphere)
    REG(GenMeshCylinder)
    REG(GenMeshTorus)
    REG(GenMeshKnot)
    REG(GenMeshHeightmap)
    REG(GenMeshCubicmap)
    //REG(LoadMaterial)
    //REG(LoadMaterialDefault)
    //REG(UnloadMaterial)
    //REG(DrawModel)
    //REG(DrawModelEx)
    //REG(DrawModelWires)
    //REG(DrawModelWiresEx)
    REG(DrawBoundingBox)
    REG(DrawBillboard)
    REG(DrawBillboardRec)
    REG(CheckCollisionSpheres)
    REG(CheckCollisionBoxes)
    REG(CheckCollisionBoxSphere)
    //REG(CheckCollisionRaySphere)
    //REG(CheckCollisionRaySphereEx)
    //REG(CheckCollisionRayBox)
    //REG(LoadText)
    REG(LoadShader)
    REG(LoadShaderFromMemory)
    REG(UnloadShader)
    //REG(GetShaderDefault)
    //REG(GetTextureDefault)
    REG(GetShaderLocation)
    REG(SetShaderValue)
    //REG(SetShaderValuei)
    REG(SetShaderValueMatrix)
    //REG(SetMatrixProjection)
    //REG(SetMatrixModelview)
    //REG(GetMatrixModelview)
    //REG(GenTextureCubemap)
    //REG(GenTextureIrradiance)
    //REG(GenTexturePrefilter)
    //REG(GenTextureBRDF)
    REG(BeginShaderMode)
    REG(EndShaderMode)
    REG(BeginBlendMode)
    REG(EndBlendMode)
    #if 0
    REG(GetVrDeviceInfo)
    REG(InitVrSimulator)
    REG(CloseVrSimulator)
    REG(IsVrSimulatorReady)
    REG(SetVrDistortionShader)
    REG(UpdateVrTracking)
    REG(ToggleVrMode)
    REG(BeginVrDrawing)
    REG(EndVrDrawing)
    #endif
    REG(InitAudioDevice)
    REG(CloseAudioDevice)
    REG(IsAudioDeviceReady)
    REG(SetMasterVolume)
    REG(LoadWave)
    //REG(LoadWaveEx)
    REG(LoadSound)
    REG(LoadSoundFromWave)
    REG(UpdateSound)
    REG(UnloadWave)
    REG(UnloadSound)
    REG(PlaySound)
    REG(PauseSound)
    REG(ResumeSound)
    REG(StopSound)
    REG(IsSoundPlaying)
    REG(SetSoundVolume)
    REG(SetSoundPitch)
    REG(WaveFormat)
    REG(WaveCopy)
    REG(WaveCrop)
    //REG(GetWaveData)
    REG(LoadMusicStream)
    REG(UnloadMusicStream)
    REG(PlayMusicStream)
    REG(UpdateMusicStream)
    REG(StopMusicStream)
    REG(PauseMusicStream)
    REG(ResumeMusicStream)
    //REG(IsMusicPlaying)
    REG(SetMusicVolume)
    REG(SetMusicPitch)
    //REG(SetMusicLoopCount)
    REG(GetMusicTimeLength)
    REG(GetMusicTimePlayed)
    //REG(InitAudioStream)
    //REG(UpdateAudioStream)
    //REG(CloseAudioStream)
    //REG(IsAudioBufferProcessed)
    REG(PlayAudioStream)
    REG(PauseAudioStream)
    REG(ResumeAudioStream)
    REG(IsAudioStreamPlaying)
    REG(StopAudioStream)
    REG(SetAudioStreamVolume)
    REG(SetAudioStreamPitch)

    { NULL, NULL }  // sentinel: end signal
};

// Register raylib Lua functionality
static void rLuaRegisterFunctions(const char *opt_table)
{
    lua_pushglobaltable(L);

    luaL_setfuncs(L, raylib_functions, 0);
}

//----------------------------------------------------------------------------------
// raylib Lua API
//----------------------------------------------------------------------------------

#if 0
// Initialize Lua system
RLUADEF void rLuaInitDevice(void)
{
    mainLuaState = luaL_newstate();
    L = mainLuaState;
#endif

int luaopen_raylib_lua(lua_State* mainLuaState) {

  L = mainLuaState;
  
    LuaStartEnum();
    LuaSetEnum("SHOW_LOGO", 1);
    LuaSetEnum("FULLSCREEN_MODE", 2);
    LuaSetEnum("WINDOW_RESIZABLE", 4);
    LuaSetEnum("WINDOW_DECORATED", 8);
    LuaSetEnum("WINDOW_TRANSPARENT", 16);
    LuaSetEnum("MSAA_4X_HINT", 32);
    LuaSetEnum("VSYNC_HINT", 64);
    LuaEndEnum("FLAG");

    LuaStartEnum();
    LuaSetEnum("SPACE", 32);
    LuaSetEnum("ESCAPE", 256);
    LuaSetEnum("ENTER", 257);
    LuaSetEnum("BACKSPACE", 259);
    LuaSetEnum("RIGHT", 262);
    LuaSetEnum("LEFT", 263);
    LuaSetEnum("DOWN", 264);
    LuaSetEnum("UP", 265);
    LuaSetEnum("F1", 290);
    LuaSetEnum("F2", 291);
    LuaSetEnum("F3", 292);
    LuaSetEnum("F4", 293);
    LuaSetEnum("F5", 294);
    LuaSetEnum("F6", 295);
    LuaSetEnum("F7", 296);
    LuaSetEnum("F8", 297);
    LuaSetEnum("F9", 298);
    LuaSetEnum("F10", 299);
    LuaSetEnum("LEFT_SHIFT", 340);
    LuaSetEnum("LEFT_CONTROL", 341);
    LuaSetEnum("LEFT_ALT", 342);
    LuaSetEnum("RIGHT_SHIFT", 344);
    LuaSetEnum("RIGHT_CONTROL", 345);
    LuaSetEnum("RIGHT_ALT", 346);
    LuaSetEnum("ZERO", 48);
    LuaSetEnum("ONE", 49);
    LuaSetEnum("TWO", 50);
    LuaSetEnum("THREE", 51);
    LuaSetEnum("FOUR", 52);
    LuaSetEnum("FIVE", 53);
    LuaSetEnum("SIX", 54);
    LuaSetEnum("SEVEN", 55);
    LuaSetEnum("EIGHT", 56);
    LuaSetEnum("NINE", 57);
    LuaSetEnum("A", 65);
    LuaSetEnum("B", 66);
    LuaSetEnum("C", 67);
    LuaSetEnum("D", 68);
    LuaSetEnum("E", 69);
    LuaSetEnum("F", 70);
    LuaSetEnum("G", 71);
    LuaSetEnum("H", 72);
    LuaSetEnum("I", 73);
    LuaSetEnum("J", 74);
    LuaSetEnum("K", 75);
    LuaSetEnum("L", 76);
    LuaSetEnum("M", 77);
    LuaSetEnum("N", 78);
    LuaSetEnum("O", 79);
    LuaSetEnum("P", 80);
    LuaSetEnum("Q", 81);
    LuaSetEnum("R", 82);
    LuaSetEnum("S", 83);
    LuaSetEnum("T", 84);
    LuaSetEnum("U", 85);
    LuaSetEnum("V", 86);
    LuaSetEnum("W", 87);
    LuaSetEnum("X", 88);
    LuaSetEnum("Y", 89);
    LuaSetEnum("Z", 90);
    LuaEndEnum("KEY");

    LuaStartEnum();
    LuaSetEnum("LEFT_BUTTON", 0);
    LuaSetEnum("RIGHT_BUTTON", 1);
    LuaSetEnum("MIDDLE_BUTTON", 2);
    LuaEndEnum("MOUSE");

    lua_pushglobaltable(L);
    LuaSetEnumColor("LIGHTGRAY", LIGHTGRAY);
    LuaSetEnumColor("GRAY", GRAY);
    LuaSetEnumColor("DARKGRAY", DARKGRAY);
    LuaSetEnumColor("YELLOW", YELLOW);
    LuaSetEnumColor("GOLD", GOLD);
    LuaSetEnumColor("ORANGE", ORANGE);
    LuaSetEnumColor("PINK", PINK);
    LuaSetEnumColor("RED", RED);
    LuaSetEnumColor("MAROON", MAROON);
    LuaSetEnumColor("GREEN", GREEN);
    LuaSetEnumColor("LIME", LIME);
    LuaSetEnumColor("DARKGREEN", DARKGREEN);
    LuaSetEnumColor("SKYBLUE", SKYBLUE);
    LuaSetEnumColor("BLUE", BLUE);
    LuaSetEnumColor("DARKBLUE", DARKBLUE);
    LuaSetEnumColor("PURPLE", PURPLE);
    LuaSetEnumColor("VIOLET", VIOLET);
    LuaSetEnumColor("DARKPURPLE", DARKPURPLE);
    LuaSetEnumColor("BEIGE", BEIGE);
    LuaSetEnumColor("BROWN", BROWN);
    LuaSetEnumColor("DARKBROWN", DARKBROWN);
    LuaSetEnumColor("WHITE", WHITE);
    LuaSetEnumColor("BLACK", BLACK);
    LuaSetEnumColor("BLANK", BLANK);
    LuaSetEnumColor("MAGENTA", MAGENTA);
    LuaSetEnumColor("RAYWHITE", RAYWHITE);
    lua_pop(L, 1);

    #if 0
    LuaStartEnum();
    LuaSetEnum("UNCOMPRESSED_GRAYSCALE", UNCOMPRESSED_GRAYSCALE);
    LuaSetEnum("UNCOMPRESSED_GRAY_ALPHA", UNCOMPRESSED_GRAY_ALPHA);
    LuaSetEnum("UNCOMPRESSED_R5G6B5", UNCOMPRESSED_R5G6B5);
    LuaSetEnum("UNCOMPRESSED_R8G8B8", UNCOMPRESSED_R8G8B8);
    LuaSetEnum("UNCOMPRESSED_R5G5B5A1", UNCOMPRESSED_R5G5B5A1);
    LuaSetEnum("UNCOMPRESSED_R4G4B4A4", UNCOMPRESSED_R4G4B4A4);
    LuaSetEnum("UNCOMPRESSED_R8G8B8A8", UNCOMPRESSED_R8G8B8A8);
    LuaSetEnum("COMPRESSED_DXT1_RGB", COMPRESSED_DXT1_RGB);
    LuaSetEnum("COMPRESSED_DXT1_RGBA", COMPRESSED_DXT1_RGBA);
    LuaSetEnum("COMPRESSED_DXT3_RGBA", COMPRESSED_DXT3_RGBA);
    LuaSetEnum("COMPRESSED_DXT5_RGBA", COMPRESSED_DXT5_RGBA);
    LuaSetEnum("COMPRESSED_ETC1_RGB", COMPRESSED_ETC1_RGB);
    LuaSetEnum("COMPRESSED_ETC2_RGB", COMPRESSED_ETC2_RGB);
    LuaSetEnum("COMPRESSED_ETC2_EAC_RGBA", COMPRESSED_ETC2_EAC_RGBA);
    LuaSetEnum("COMPRESSED_PVRT_RGB", COMPRESSED_PVRT_RGB);
    LuaSetEnum("COMPRESSED_PVRT_RGBA", COMPRESSED_PVRT_RGBA);
    LuaSetEnum("COMPRESSED_ASTC_4x4_RGBA", COMPRESSED_ASTC_4x4_RGBA);
    LuaSetEnum("COMPRESSED_ASTC_8x8_RGBA", COMPRESSED_ASTC_8x8_RGBA);
    LuaEndEnum("TextureFormat");
    #endif

    LuaStartEnum();
    LuaSetEnum("ALPHA", BLEND_ALPHA);
    LuaSetEnum("ADDITIVE", BLEND_ADDITIVE);
    LuaSetEnum("MULTIPLIED", BLEND_MULTIPLIED);
    LuaEndEnum("BlendMode");

    #if 0
    LuaStartEnum();
    LuaSetEnum("POINT", FILTER_POINT);
    LuaSetEnum("BILINEAR", FILTER_BILINEAR);
    LuaSetEnum("TRILINEAR", FILTER_TRILINEAR);
    LuaSetEnum("ANISOTROPIC_4X", FILTER_ANISOTROPIC_4X);
    LuaSetEnum("ANISOTROPIC_8X", FILTER_ANISOTROPIC_8X);
    LuaSetEnum("ANISOTROPIC_16X", FILTER_ANISOTROPIC_16X);
    LuaEndEnum("TextureFilter");
    #endif

    LuaStartEnum();
    LuaSetEnum("NONE", GESTURE_NONE);
    LuaSetEnum("TAP", GESTURE_TAP);
    LuaSetEnum("DOUBLETAP", GESTURE_DOUBLETAP);
    LuaSetEnum("HOLD", GESTURE_HOLD);
    LuaSetEnum("DRAG", GESTURE_DRAG);
    LuaSetEnum("SWIPE_RIGHT", GESTURE_SWIPE_RIGHT);
    LuaSetEnum("SWIPE_LEFT", GESTURE_SWIPE_LEFT);
    LuaSetEnum("SWIPE_UP", GESTURE_SWIPE_UP);
    LuaSetEnum("SWIPE_DOWN", GESTURE_SWIPE_DOWN);
    LuaSetEnum("PINCH_IN", GESTURE_PINCH_IN);
    LuaSetEnum("PINCH_OUT", GESTURE_PINCH_OUT);
    LuaEndEnum("Gestures");

    LuaStartEnum();
    LuaSetEnum("CUSTOM", CAMERA_CUSTOM);
    LuaSetEnum("FREE", CAMERA_FREE);
    LuaSetEnum("ORBITAL", CAMERA_ORBITAL);
    LuaSetEnum("FIRST_PERSON", CAMERA_FIRST_PERSON);
    LuaSetEnum("THIRD_PERSON", CAMERA_THIRD_PERSON);
    LuaEndEnum("CameraMode");

    #if 0
    LuaStartEnum();
    LuaSetEnum("DEFAULT_DEVICE", HMD_DEFAULT_DEVICE);
    LuaSetEnum("OCULUS_RIFT_DK2", HMD_OCULUS_RIFT_DK2);
    LuaSetEnum("OCULUS_RIFT_CV1", HMD_OCULUS_RIFT_CV1);
    LuaSetEnum("VALVE_HTC_VIVE", HMD_VALVE_HTC_VIVE);
    LuaSetEnum("SAMSUNG_GEAR_VR", HMD_SAMSUNG_GEAR_VR);
    LuaSetEnum("GOOGLE_CARDBOARD", HMD_GOOGLE_CARDBOARD);
    LuaSetEnum("SONY_PLAYSTATION_VR", HMD_SONY_PLAYSTATION_VR);
    LuaSetEnum("RAZER_OSVR", HMD_RAZER_OSVR);
    LuaSetEnum("FOVE_VR", HMD_FOVE_VR);
    LuaEndEnum("VrDevice");
    #endif

    #if 0
    lua_pushglobaltable(L);
    LuaSetEnum("INFO", INFO);
    LuaSetEnum("ERROR", ERROR);
    LuaSetEnum("WARNING", WARNING);
    LuaSetEnum("DEBUG", DEBUG);
    LuaSetEnum("OTHER", OTHER);
    lua_pop(L, 1);
    #endif

    LuaPush_bool(L, true);
#if defined(PLATFORM_DESKTOP)
    lua_setglobal(L, "PLATFORM_DESKTOP");
#elif defined(PLATFORM_ANDROID)
    lua_setglobal(L, "PLATFORM_ANDROID");
#elif defined(PLATFORM_RPI)
    lua_setglobal(L, "PLATFORM_RPI");
#elif defined(PLATFORM_WEB)
    lua_setglobal(L, "PLATFORM_WEB");
#endif

    //luaL_openlibs(L);
    LuaBuildOpaqueMetatables();

    rLuaRegisterFunctions(0);        // Register Lua raylib functions

    return 0;
}

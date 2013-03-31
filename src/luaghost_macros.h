inline const char * ghost_check_string(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TSTRING)
		luaL_typerror(L, n, lua_typename(L, LUA_TSTRING));
	return (const char *) lua_tostring(L, n);
}

inline const void * ghost_check_lightuserdate(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
		luaL_typerror(L, n, "lightuserdata");
	return (const void *) lua_touserdata(L, n);
}

#define CONST(name)\
	lua_pushstring(L, #name);\
	lua_pushnumber(L, name);\
	lua_settable(L, LUA_GLOBALSINDEX);

#define PTR(name)\
	lua_pushstring(L, #name);\
	lua_pushlightuserdata(L, name);\
	lua_settable(L, LUA_GLOBALSINDEX);

#define FUN_SPEC(name)

#define FUNC(name)\
	lua_register(L, #name, L ## name);

/**
typedef void*				GHOST_TUserDataPtr;
**/

inline GHOST_TInt8 check_GHOST_TInt8(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TInt8)");
	return (GHOST_TInt8) lua_tonumber(L, n);
}

inline GHOST_TUns8 check_GHOST_TUns8(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TUns8)");
	return (GHOST_TUns8) lua_tonumber(L, n);
}

inline GHOST_TInt16 check_GHOST_TInt16(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TInt16)");
	return (GHOST_TInt16) lua_tonumber(L, n);
}

inline GHOST_TUns16 check_GHOST_TUns16(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TUns16)");
	return (GHOST_TUns16) lua_tonumber(L, n);
}

inline GHOST_TInt32 check_GHOST_TInt32(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TInt32)");
	return (GHOST_TInt32) lua_tonumber(L, n);
}

inline GHOST_TUns32 check_GHOST_TUns32(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TUns32)");
	return (GHOST_TUns32) lua_tonumber(L, n);
}

inline GHOST_TInt64 check_GHOST_TInt64(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TInt64)");
	return (GHOST_TInt64) lua_tonumber(L, n);
}

inline GHOST_TUns64 check_GHOST_TUns64(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TUns64)");
	return (GHOST_TUns64) lua_tonumber(L, n);
}

inline GHOST_TStandardCursor check_GHOST_TStandardCursor(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TStandardCursor)");
	return (GHOST_TStandardCursor) lua_tonumber(L, n);
}

inline GHOST_TDrawingContextType check_GHOST_TDrawingContextType(lua_State *L, int n)
{
	if(lua_type(L, n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TDrawingContextType)");
	return (GHOST_TDrawingContextType) lua_tonumber(L, n);
}

inline GHOST_TWindowState check_GHOST_TWindowState(lua_State *L, int n)
{
	if(lua_type(L, n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TWindowState)");
	return (GHOST_TWindowState) lua_tonumber(L, n);
}

inline GHOST_TWindowOrder check_GHOST_TWindowOrder(lua_State *L, int n)
{
	if(lua_type(L, n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TWindowOrder)");
	return (GHOST_TWindowOrder) lua_tonumber(L, n);
}

inline GHOST_TModifierKeyMask check_GHOST_TModifierKeyMask(lua_State *L, int n)
{
	if(lua_type(L, n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TModifierKeyMask)");
	return (GHOST_TModifierKeyMask) lua_tonumber(L, n);
}

inline GHOST_TButtonMask check_GHOST_TButtonMask(lua_State *L, int n)
{
	if(lua_type(L, n) != LUA_TNUMBER)
		luaL_typerror(L, n, "number(GHOST_TButtonMask)");
	return (GHOST_TButtonMask) lua_tonumber(L, n);
}

inline GHOST_TUserDataPtr check_GHOST_TUserDataPtr(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
		luaL_typerror(L, n, "GHOST_TUserDataPtr");
	return (GHOST_TUserDataPtr) lua_touserdata(L, n);
}

inline GHOST_TEventDataPtr check_GHOST_TEventDataPtr(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
		luaL_typerror(L, n, "GHOST_TEventDataPtr");
	return (GHOST_TEventDataPtr) lua_touserdata(L, n);
}

inline GHOST_SystemHandle check_GHOST_SystemHandle(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
		luaL_typerror(L, n, "GHOST_SystemHandle");
	return (GHOST_SystemHandle) lua_touserdata(L, n);
}

inline GHOST_TimerTaskHandle check_GHOST_TimerTaskHandle(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
		luaL_typerror(L, n, "GHOST_TimerTaskHandle");
	return (GHOST_TimerTaskHandle) lua_touserdata(L, n);
}

inline GHOST_WindowHandle check_GHOST_WindowHandle(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
		luaL_typerror(L, n, "GHOST_WindowHandle");
	return (GHOST_WindowHandle) lua_touserdata(L, n);
}

inline GHOST_EventHandle check_GHOST_EventHandle(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
		luaL_typerror(L, n, "GHOST_EventHandle");
	return (GHOST_EventHandle) lua_touserdata(L, n);
}

inline GHOST_RectangleHandle check_GHOST_RectangleHandle(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
		luaL_typerror(L, n, "GHOST_RectangleHandle");
	return (GHOST_RectangleHandle) lua_touserdata(L, n);
}

inline GHOST_EventConsumerHandle check_GHOST_EventConsumerHandle(lua_State *L, int n)
{
	if (lua_type(L,n) != LUA_TLIGHTUSERDATA)
		luaL_typerror(L, n, "GHOST_EventConsumerHandle");
	return (GHOST_EventConsumerHandle) lua_touserdata(L, n);
}

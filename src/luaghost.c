/* ======================================================================
** luaghost.c - Copyright (C) 2006 Nathan Letwory
** ======================================================================
***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GHOST_C-api.h"

#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <assert.h>
#include "luaghost_macros.h"

//#define MYNAME "luaghost"
#define VERSION "0.5"

lua_State *ref_L = NULL;

#if 0 // enable only when stack needs to be inspected
static void stackDump (lua_State *L) {
	int i;
	int top = lua_gettop(L);
	for (i = 1; i <= top; i++) {  /* repeat for each level */
		int t = lua_type(L, i);
		switch (t) {
			case LUA_TSTRING:  /* strings */
				printf("`%s'", lua_tostring(L, i));
				break;

			case LUA_TBOOLEAN:  /* booleans */
				printf(lua_toboolean(L, i) ? "true" : "false");
				break;

			case LUA_TNUMBER:  /* numbers */
				printf("%g", lua_tonumber(L, i));
				break;

			default:  /* other values */
				printf("%s", lua_typename(L, t));
				break;

		}
		printf("  ");  /* put a separator */
	}
	printf("\n");  /* end the listing */
}
#endif

LUA_API int LGHOST_CreateSystem(lua_State *L)
{
	assert(L == ref_L);

	GHOST_SystemHandle sysh = GHOST_CreateSystem();
	lua_pushlightuserdata(L, sysh);
	return 1;
}

LUA_API int LGHOST_DisposeSystem(lua_State *L)
{
	assert(L == ref_L);

	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_TSuccess success = GHOST_DisposeSystem(sysh);
	lua_pushnumber(L, (lua_Number)(success));
	return 1;
}

typedef struct CW {
	int index;
	GHOST_TUserDataPtr data;
} ConsumerWrapper;

static int consumer_func(GHOST_EventHandle hEvent, GHOST_TUserDataPtr UserData)
{
	int handled;

	ConsumerWrapper *cwrapper = (ConsumerWrapper *)UserData;

	lua_pushstring(ref_L, "luaghost");
	lua_gettable(ref_L, LUA_GLOBALSINDEX);

	lua_pushstring(ref_L, "consumer");
	lua_gettable(ref_L, -2);

	lua_rawgeti(ref_L, -1, cwrapper->index);

	lua_pushlightuserdata(ref_L, hEvent);
	if(cwrapper->data != NULL)
		lua_pushlightuserdata(ref_L, cwrapper->data);
	else
		lua_pushnil(ref_L);

	lua_call(ref_L, 2, 1);

	handled = luaL_checkint(ref_L, -1);
	/* TODO: issue nice error message:
		 event handler must return integer 0:1
		 */

	lua_pop(ref_L, -1);

	return handled;
}

LUA_API int LGHOST_CreateEventConsumer(lua_State *L)
{
	assert(L == ref_L);

	int next_free, index, new_next_free;
	ConsumerWrapper *cwrapper = NULL;

	//GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, -2);
	cwrapper = (ConsumerWrapper *)malloc(sizeof(ConsumerWrapper));
	cwrapper->data = lua_isnil(L, -1) ? NULL : check_GHOST_TUserDataPtr(L, -1);

	/* get luaghost */
	lua_pushstring(L, "luaghost");
	lua_gettable(L, LUA_GLOBALSINDEX);

	/* get luaghost.consumer */
	lua_pushstring(L, "consumer");
	lua_gettable(L, -2);

	lua_pushstring(L, "next_free");
	lua_gettable(L, -2);
	next_free = luaL_checknumber(L, -1);

	if(next_free < 0 ) {
		/* no more free? abs(next_free) == size */

		/* consumer[-next_free] = func*/
		lua_pushvalue(L, -5);
		lua_rawseti(L, -3, -next_free);

		/* next_free = next_free -1 */
		lua_pushstring(L, "next_free");
		lua_pushnumber(L, next_free - 1);
		lua_rawset(L, -4);

		index = -next_free;
	} else {
		/* can reuse some index */

		/* get consumer.next_free, i.e. our new_next_free */
		lua_rawgeti(L, -3, next_free);
		new_next_free = luaL_checknumber(L, -1);

		/* set consumer[next_free] = func */
		lua_pushvalue(L, -5);
		lua_rawseti(L, -3, next_free);

		/* next_free = new_next_free */
		lua_pushstring(L, "next_free");
		lua_pushnumber(L, new_next_free);
		lua_rawset(L, -5);

		index = next_free;
	}

	cwrapper->index = index;
	GHOST_EventConsumerHandle consumerh = GHOST_CreateEventConsumer(consumer_func, cwrapper);
	lua_pushlightuserdata(L, consumerh);
	return 1;
}

LUA_API int LGHOST_DisposeEventConsumer(lua_State *L)
{
	assert(L == ref_L);

	GHOST_EventConsumerHandle consumerh = check_GHOST_EventConsumerHandle(L, 1);
	GHOST_TSuccess success = GHOST_DisposeEventConsumer(consumerh);
	lua_pushnumber(L, (lua_Number)(success));
	return 1;
}

typedef struct TW {
	int index;
	GHOST_TUserDataPtr data;
} TimerWrapper;

static void timer_func(GHOST_TimerTaskHandle timerh, GHOST_TUns64 time)
{
	TimerWrapper *twrapper = GHOST_GetTimerTaskUserData(timerh);

	lua_pushstring(ref_L, "luaghost");
	lua_gettable(ref_L, LUA_GLOBALSINDEX);

	lua_pushstring(ref_L, "timer");
	lua_gettable(ref_L, -2);

	lua_rawgeti(ref_L, -1, twrapper->index);

	lua_pushlightuserdata(ref_L, timerh);
	lua_pushnumber(ref_L, (lua_Number)time);

	lua_call(ref_L, 2, 0);

	return;
}

LUA_API int LGHOST_InstallTimer(lua_State *L)
{
	assert(L == ref_L);

	int next_free, index, new_next_free;
	TimerWrapper *twrapper = NULL;

	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, -5);
	GHOST_TUns64 delay = check_GHOST_TUns64(L, -4);
	GHOST_TUns64 interval = check_GHOST_TUns64(L, -3);
	twrapper = (TimerWrapper *)malloc(sizeof(TimerWrapper));
	twrapper->data = lua_isnil(L, -1) ? NULL : check_GHOST_TUserDataPtr(L, -1);

	/* get luaghost */
	lua_pushstring(L, "luaghost");
	lua_gettable(L, LUA_GLOBALSINDEX);

	/* get luaghost.timer */
	lua_pushstring(L, "timer");
	lua_gettable(L, -2);

	lua_pushstring(L, "next_free");
	lua_gettable(L, -2);
	next_free = luaL_checknumber(L, -1);

	if(next_free < 0 ) {
		/* no more free? abs(next_free) == size */

		/* timer[-next_free] = func*/
		lua_pushvalue(L, -5);
		lua_rawseti(L, -3, -next_free);

		/* next_free = next_free -1 */
		lua_pushstring(L, "next_free");
		lua_pushnumber(L, next_free - 1);
		lua_rawset(L, -4);

		index = -next_free;
	} else {
		/* can reuse some index */

		/* get timer.next_free, i.e. our new_next_free */
		lua_rawgeti(L, -3, next_free);
		new_next_free = luaL_checknumber(L, -1);

		/* set timer[next_free] = FUNCc */
		lua_pushvalue(L, -5);
		lua_rawseti(L, -3, next_free);

		/* next_free = new_next_free */
		lua_pushstring(L, "next_free");
		lua_pushnumber(L, new_next_free);
		lua_rawset(L, -5);

		index = next_free;
	}

	twrapper->index = index;
	GHOST_TimerTaskHandle timerh = GHOST_InstallTimer(sysh, delay, interval, timer_func, (GHOST_TUserDataPtr)twrapper);
	lua_pushlightuserdata(L, timerh);
	return 1;
}

LUA_API int LGHOST_RemoveTimer(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_TimerTaskHandle htask = check_GHOST_TimerTaskHandle(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_RemoveTimer(sysh, htask));

	/* mem leak: need to solve freeing lua-ghost layer TimerWrapper */

	return 1;
}

LUA_API int LGHOST_GetMilliSeconds(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_GetMilliSeconds(sysh));
	return 1;
}

LUA_API int LGHOST_GetTimerTaskUserData(lua_State *L)
{
	assert(L == ref_L);
	GHOST_TimerTaskHandle htask = check_GHOST_TimerTaskHandle(L, 1);
	TimerWrapper *twrapper = GHOST_GetTimerTaskUserData(htask);

	lua_pushlightuserdata(L, twrapper->data);

	return 1;
}

LUA_API int LGHOST_SetTimerTaskUserData(lua_State *L)
{
	assert(L == ref_L);
	GHOST_TimerTaskHandle htask = check_GHOST_TimerTaskHandle(L, 1);
	GHOST_TUserDataPtr data = check_GHOST_TUserDataPtr(L, 2);
	TimerWrapper *twrapper = GHOST_GetTimerTaskUserData(htask);

	if(twrapper == NULL) {
		twrapper = (TimerWrapper *)malloc(sizeof(TimerWrapper));
	}
	twrapper->data = data;

	return 0;
}

LUA_API int LGHOST_AddEventConsumer(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_EventConsumerHandle consumerh = check_GHOST_EventConsumerHandle(L, 2);
	GHOST_AddEventConsumer(sysh, consumerh);
	return 0;
}

LUA_API int LGHOST_ProcessEvents(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	int waitfor = luaL_checkint(L, 2);
	lua_pushnumber(L, (lua_Number)(GHOST_ProcessEvents(sysh, waitfor)));
	return 1;
}

LUA_API int LGHOST_DispatchEvents(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	lua_pushnumber(L, (lua_Number)(GHOST_DispatchEvents(sysh)));
	return 1;
}

LUA_API int LGHOST_GetNumDisplays(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	lua_pushnumber(L, (lua_Number)(GHOST_GetNumDisplays(sysh)));
	
	return 1;
}

LUA_API int LGHOST_GetMainDisplayDimensions(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_TUns32 width, height;

	GHOST_GetMainDisplayDimensions(sysh, &width, &height);
	lua_pushnumber(L, (lua_Number)(width));
	lua_pushnumber(L, (lua_Number)(height));

	return 2;
}

LUA_API int LGHOST_CreateWindow(lua_State *L)
{
	assert(L == ref_L);

	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	char *title = (char *)luaL_checkstring(L, 2);
	GHOST_TInt32 left = (GHOST_TInt32)luaL_checkint(L, 3);
	GHOST_TInt32 top = (GHOST_TInt32)luaL_checkint(L, 4);
	GHOST_TUns32 width = (GHOST_TUns32)luaL_checkint(L, 5);
	GHOST_TUns32 height = (GHOST_TUns32)luaL_checkint(L, 6);
	GHOST_TWindowState state = (GHOST_TWindowState)luaL_checkint(L, 7);
	GHOST_TDrawingContextType type = (GHOST_TDrawingContextType)luaL_checkint(L, 8);
	const int stereoVisual = 0;

	GHOST_WindowHandle winh = GHOST_CreateWindow(sysh, title,
			left, top,
			width, height,
			state, type,
			stereoVisual);

	lua_pushlightuserdata(L, winh);
	return 1;
}

LUA_API int LGHOST_DisposeWindow(lua_State *L)
{
	assert(L == ref_L);

	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 2);

	GHOST_TSuccess success = GHOST_DisposeWindow(sysh, winh);
	lua_replace(L, 2);
	lua_pushnumber(L, (lua_Number)success);
	return 1;
}

LUA_API int LGHOST_GetWindowUserData(lua_State *L)
{
	assert(L == ref_L);

	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);

	lua_pushlightuserdata(L, GHOST_GetWindowUserData(winh));
	return 1;
}

LUA_API int LGHOST_SetWindowUserData(lua_State *L)
{
	assert(L == ref_L);

	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TUserDataPtr data = check_GHOST_TUserDataPtr(L, 2);

	GHOST_SetWindowUserData(winh, lua_isnil(L, 2) ? NULL : data);
	return 0;
}

LUA_API int LGHOST_ValidWindow(lua_State *L)
{
	assert(L == ref_L);

	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 2);

	GHOST_TSuccess success = GHOST_ValidWindow(sysh, winh);
	lua_pushnumber(L, (lua_Number)success);
	return 1;
}

LUA_API int LGHOST_InvalidateWindow(lua_State *L)
{
	assert(L == ref_L);

	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_InvalidateWindow(winh);

	return 0;
}

LUA_API int LGHOST_BeginFullScreen(lua_State *L)
{
	assert(L == ref_L);
	GHOST_DisplaySetting setting;

	int stereo = luaL_checkint(L, -1);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, -3);

	lua_pushstring(L, "xPixels");
	lua_gettable(L, -3);

	lua_pushstring(L, "yPixels");
	lua_gettable(L, -4);

	lua_pushstring(L, "bpp");
	lua_gettable(L, -5);

	lua_pushstring(L, "frequency");
	lua_gettable(L, -6);

	setting.xPixels = check_GHOST_TUns32(L, -4);
	setting.yPixels = check_GHOST_TUns32(L, -3);
	setting.bpp = check_GHOST_TUns32(L, -2);
	setting.frequency = check_GHOST_TUns32(L, -1);

	lua_pushlightuserdata(L, GHOST_BeginFullScreen(sysh, &setting, stereo)); /* stereo flag for now on 0 */
	return 1;
}

LUA_API int LGHOST_EndFullScreen(lua_State *L)
{
	assert(L == ref_L);

	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_EndFullScreen(sysh));
	return 1;
}

LUA_API int LGHOST_GetFullScreen(lua_State *L)
{
	assert(L == ref_L);

	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_GetFullScreen(sysh));
	return 1;
}

LUA_API int LGHOST_GetEventType(lua_State *L)
{
	assert(L == ref_L);

	GHOST_EventHandle eventh = check_GHOST_EventHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_GetEventType(eventh));
	return 1;
}

LUA_API int LGHOST_GetEventWindow(lua_State *L)
{
	assert(L == ref_L);
	GHOST_EventHandle eventh = check_GHOST_EventHandle(L, 1);
	lua_pushlightuserdata(L, GHOST_GetEventWindow(eventh));
	return 1;
}

LUA_API int LGHOST_GetModifierKeyState(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_TModifierKeyMask mask = check_GHOST_TModifierKeyMask(L, 2);
	int isdown;

	GHOST_GetModifierKeyState(sysh, mask, &isdown);
	lua_pushnumber(L, (lua_Number)isdown);
	return 1;
}

LUA_API int LGHOST_GetButtonState(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_TButtonMask mask = check_GHOST_TButtonMask(L, 2);
	int isdown;

	GHOST_GetButtonState(sysh, mask, &isdown);
	lua_pushnumber(L, (lua_Number)isdown);
	return 1;
}

LUA_API int LGHOST_GetEventTime(lua_State *L)
{
	assert(L == ref_L);
	GHOST_EventHandle eventh = check_GHOST_EventHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_GetEventTime(eventh));

	return 1;
}

LUA_API int LGHOST_GetEventData(lua_State *L)
{
	assert(L == ref_L);
	const char *data_type = ghost_check_string(L, 1);
	GHOST_EventHandle eventh = check_GHOST_EventHandle(L, 2);

	if(strcmp(data_type, "wheel") == 0) {
		GHOST_TEventWheelData *wheel = (GHOST_TEventWheelData *)GHOST_GetEventData(eventh);
		lua_pushnumber(L, (lua_Number)wheel->z);
		return 1;
	} else if(strcmp(data_type, "button") == 0) {
		GHOST_TEventButtonData *button = (GHOST_TEventButtonData *)GHOST_GetEventData(eventh);
		lua_pushnumber(L, (lua_Number)button->button);
		return 1;
	} else if(strcmp(data_type, "key") == 0) {
		GHOST_TEventKeyData *key = (GHOST_TEventKeyData *)GHOST_GetEventData(eventh);
		lua_pushnumber(L, (lua_Number)key->key);
		if(key->ascii=='\0') {
			lua_pushnil(L);
		} else {
			lua_pushfstring(L, "%c", key->ascii);
		}
		return 2;
	} else if(strcmp(data_type, "cursor") == 0) {
		GHOST_TEventCursorData *cursor = (GHOST_TEventCursorData *)GHOST_GetEventData(eventh);
		lua_pushnumber(L, (lua_Number)cursor->x);
		lua_pushnumber(L, (lua_Number)cursor->y);
		return 2;
	} 

	return 0;

}

LUA_API int LGHOST_GetValid(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_GetValid(winh));
	return 1;
}

LUA_API int LGHOST_SetDrawingContextType(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TDrawingContextType dct = check_GHOST_TDrawingContextType(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_SetDrawingContextType(winh, dct));
	return 1;
}

LUA_API int LGHOST_GetDrawingContextType(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_GetDrawingContextType(winh));
	return 1;
}

LUA_API int LGHOST_SetTitle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	char *title = (char *)ghost_check_string(L, 2);
	GHOST_SetTitle(winh, title);
	return 0;
}

LUA_API int LGHOST_GetTitle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);	
	lua_pushstring(L, (const char *)GHOST_GetTitle(winh));
	return 1;
}

LUA_API int LGHOST_ActivateWindowDrawingContext(lua_State *L)
{
	assert(L == ref_L);

	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_ActivateWindowDrawingContext(winh));
	return 1;
}

LUA_API int LGHOST_SwapWindowBuffers(lua_State *L)
{
	assert(L == ref_L);

	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_SwapWindowBuffers(winh));
	return 1;
}

LUA_API int LGHOST_GetClientBounds(lua_State *L)
{
	assert(L == ref_L);

	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	lua_pushlightuserdata(L, GHOST_GetClientBounds(winh));
	return 1;
}

LUA_API int LGHOST_GetWidthRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_GetWidthRectangle(recth));
	return 1;
}

LUA_API int LGHOST_GetHeightRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_GetHeightRectangle(recth));
	return 1;
}

LUA_API int LGHOST_DisposeRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	GHOST_DisposeRectangle(recth);
	lua_pop(L, 1);
	return 0;
}

LUA_API int LGHOST_GetWindowBounds(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	lua_pushlightuserdata(L, GHOST_GetWindowBounds(winh));
	return 1;
}

LUA_API int LGHOST_SetClientWidth(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TUns32 width = check_GHOST_TUns32(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_SetClientWidth(winh, width));
	return 1;
}

LUA_API int LGHOST_SetClientHeight(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TUns32 width = check_GHOST_TUns32(L, 2);
	GHOST_TUns32 height = check_GHOST_TUns32(L, 3);

	lua_pushnumber(L, (lua_Number)GHOST_SetClientSize(winh, width, height));
	return 1;
}

LUA_API int LGHOST_SetClientSize(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TUns32 height = check_GHOST_TUns32(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_SetClientHeight(winh, height));
	return 1;
}

LUA_API int LGHOST_ScreenToClient(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TInt32 inX = check_GHOST_TInt32(L, 2);
	GHOST_TInt32 inY = check_GHOST_TInt32(L, 3);
	GHOST_TInt32 outX, outY;

	GHOST_ScreenToClient(winh, inX, inY, &outX, &outY);
	lua_pushnumber(L, (lua_Number)outX);
	lua_pushnumber(L, (lua_Number)outY);
	return 2;
}

LUA_API int LGHOST_ClientToScreen(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TInt32 inX = check_GHOST_TInt32(L, 2);
	GHOST_TInt32 inY = check_GHOST_TInt32(L, 3);
	GHOST_TInt32 outX, outY;

	GHOST_ClientToScreen(winh, inX, inY, &outX, &outY);
	lua_pushnumber(L, (lua_Number)outX);
	lua_pushnumber(L, (lua_Number)outY);
	return 2;
}

LUA_API int LGHOST_GetWindowState(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	lua_pushnumber(L, (lua_Number)GHOST_GetWindowState(winh));
	return 1;
}

LUA_API int LGHOST_SetWindowState(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TWindowState wstate = check_GHOST_TWindowState(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_SetWindowState(winh, wstate));
	return 1;
}

LUA_API int LGHOST_SetWindowOrder(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TWindowOrder order = check_GHOST_TWindowOrder(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_SetWindowOrder(winh, order));
	return 1;
}

LUA_API int LGHOST_GetRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_TInt32 l, t, r, b;
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);

	GHOST_GetRectangle(recth, &l, &t, &r, &b);
	lua_pushnumber(L, (lua_Number)l);
	lua_pushnumber(L, (lua_Number)t);
	lua_pushnumber(L, (lua_Number)r);
	lua_pushnumber(L, (lua_Number)b);

	return 4;
}

LUA_API int LGHOST_SetRectangle(lua_State *L)
{
	assert(L == ref_L);

	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	GHOST_TInt32 l = check_GHOST_TInt32(L, 2);
	GHOST_TInt32 t = check_GHOST_TInt32(L, 3);
	GHOST_TInt32 r = check_GHOST_TInt32(L, 4);
	GHOST_TInt32 b = check_GHOST_TInt32(L, 5);

	GHOST_SetRectangle(recth, l, t, r, b);

	return 0;
}

LUA_API int LGHOST_IsEmptyRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);	
	lua_pushnumber(L, (lua_Number)GHOST_IsEmptyRectangle(recth));
	return 1;
}

LUA_API int LGHOST_IsValidRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);	
	lua_pushnumber(L, (lua_Number)GHOST_IsValidRectangle(recth));
	return 1;
}

LUA_API int LGHOST_InsetRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1); 
	GHOST_TInt32 i = check_GHOST_TInt32(L, 2);
	GHOST_InsetRectangle(recth, i);
	return 0;
}

LUA_API int LGHOST_UnionRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	GHOST_RectangleHandle rect2h = check_GHOST_RectangleHandle(L, 2); 

	GHOST_UnionRectangle(recth, rect2h);
	return 0;
}

LUA_API int LGHOST_UnionPointRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	GHOST_TInt32 x = check_GHOST_TInt32(L, 2);
	GHOST_TInt32 y = check_GHOST_TInt32(L, 3);

	GHOST_UnionPointRectangle(recth, x, y);
	return 0;
}

LUA_API int LGHOST_IsInsideRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	GHOST_TInt32 x = check_GHOST_TInt32(L, 2);
	GHOST_TInt32 y = check_GHOST_TInt32(L, 3);

	lua_pushnumber(L, (lua_Number)GHOST_IsInsideRectangle(recth, x, y));
	return 1;
}

LUA_API int LGHOST_GetRectangleVisibility(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	GHOST_RectangleHandle rect2h = check_GHOST_RectangleHandle(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_GetRectangleVisibility(recth, rect2h));
	return 1;
}

LUA_API int LGHOST_SetCenterRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	GHOST_TInt32 x = check_GHOST_TInt32(L, 2);
	GHOST_TInt32 y = check_GHOST_TInt32(L, 3);

	GHOST_SetCenterRectangle(recth, x, y);
	return 0;
}

LUA_API int LGHOST_SetRectangleCenter(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	GHOST_TInt32 x = check_GHOST_TInt32(L, 2);
	GHOST_TInt32 y = check_GHOST_TInt32(L, 3);
	GHOST_TInt32 w = check_GHOST_TInt32(L, 4);
	GHOST_TInt32 h = check_GHOST_TInt32(L, 5);

	GHOST_SetRectangleCenter(recth, x, y, w, h);
	return 0;
}

LUA_API int LGHOST_ClipRectangle(lua_State *L)
{
	assert(L == ref_L);
	GHOST_RectangleHandle recth = check_GHOST_RectangleHandle(L, 1);
	GHOST_RectangleHandle rect2h = check_GHOST_RectangleHandle(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_ClipRectangle(recth, rect2h));
	return 1;
}

LUA_API int LGHOST_SetCursorShape(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	GHOST_TStandardCursor cursor = check_GHOST_TStandardCursor(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_SetCursorShape(winh, cursor));

	return 1;
}

LUA_API int LGHOST_GetCursorShape(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);

	lua_pushnumber(L, (lua_Number)GHOST_GetCursorShape(winh));

	return 1;
}

LUA_API int LGHOST_GetCursorVisibility(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);

	lua_pushnumber(L, (lua_Number)GHOST_GetCursorVisibility(winh));

	return 1;
}

LUA_API int LGHOST_SetCursorVisibility(lua_State *L)
{
	assert(L == ref_L);
	GHOST_WindowHandle winh = check_GHOST_WindowHandle(L, 1);
	int visibility = luaL_checkint(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_SetCursorVisibility(winh, visibility));

	return 1;
}

LUA_API int LGHOST_GetCursorPosition(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_TInt32 x, y;

	GHOST_GetCursorPosition(sysh, &x, &y);

	lua_pushnumber(L, (lua_Number)x);
	lua_pushnumber(L, (lua_Number)y);

	return 2;
}

LUA_API int LGHOST_SetCursorPosition(lua_State *L)
{
	assert(L == ref_L);
	GHOST_SystemHandle sysh = check_GHOST_SystemHandle(L, 1);
	GHOST_TInt32 x = check_GHOST_TInt32(L, 2);
	GHOST_TInt32 y = check_GHOST_TInt32(L, 2);

	lua_pushnumber(L, (lua_Number)GHOST_SetCursorPosition(sysh, x, y));

	return 1;
}

/* TODO:
	FUNC(GHOST_SetCustomCursorShape)
	FUNC(GHOST_SetCustomCursorShapeEx)
	*/

void register_ghost(lua_State *L)
{
	/* Taken from GHOST_Types.h */

	/** ghost success **/
	CONST(GHOST_kFailure)
	CONST(GHOST_kSuccess)

	/** ghost visibility **/
	CONST(GHOST_kNotVisible)
	CONST(GHOST_kPartiallyVisible)
	CONST(GHOST_kFullyVisible)

	/** ghost time fire CONST **/
	CONST(GHOST_kFireTimeNever)

	/** ghost modifier key masks **/
	CONST(GHOST_kModifierKeyLeftShift)
	CONST(GHOST_kModifierKeyRightShift)
	CONST(GHOST_kModifierKeyLeftAlt)
	CONST(GHOST_kModifierKeyRightAlt)
	CONST(GHOST_kModifierKeyLeftControl)
	CONST(GHOST_kModifierKeyRightControl)
	CONST(GHOST_kModifierKeyCommand)
	CONST(GHOST_kModifierKeyNumMasks)

	/** ghost window state **/
	CONST(GHOST_kWindowStateNormal)
	CONST(GHOST_kWindowStateMaximized)
	CONST(GHOST_kWindowStateMinimized)
	CONST(GHOST_kWindowStateFullScreen)
	CONST(GHOST_kWindowState8Normal)
	CONST(GHOST_kWindowState8Maximized)
	CONST(GHOST_kWindowState8Minimized)
	CONST(GHOST_kWindowState8FullScreen)

	/** ghost window order **/
	CONST(GHOST_kWindowOrderTop)
	CONST(GHOST_kWindowOrderBottom)

	/** ghost drawing context types **/
	CONST(GHOST_kDrawingContextTypeNone)
	CONST(GHOST_kDrawingContextTypeOpenGL)

	/** ghost button masks **/
	CONST(GHOST_kButtonMaskLeft)
	CONST(GHOST_kButtonMaskMiddle)
	CONST(GHOST_kButtonMaskRight)
	CONST(GHOST_kButtonNumMasks)

	/** ghost event types **/
	CONST(GHOST_kEventUnknown)
	CONST(GHOST_kEventCursorMove)
	CONST(GHOST_kEventButtonDown)
	CONST(GHOST_kEventButtonUp)
	CONST(GHOST_kEventWheel)

	CONST(GHOST_kEventKeyDown)
	CONST(GHOST_kEventKeyUp)
	/*CONST(GHOST_kEventKeyAuto)*/
	CONST(GHOST_kEventQuit)

	CONST(GHOST_kEventWindowClose)
	CONST(GHOST_kEventWindowActivate)
	CONST(GHOST_kEventWindowDeactivate)
	CONST(GHOST_kEventWindowUpdate)
	CONST(GHOST_kEventWindowSize)

	CONST(GHOST_kNumEventTypes)

	/** ghost cursors **/
	CONST(GHOST_kStandardCursorFirstCursor)
	CONST(GHOST_kStandardCursorDefault)
	CONST(GHOST_kStandardCursorRightArrow)
	CONST(GHOST_kStandardCursorLeftArrow)
	CONST(GHOST_kStandardCursorInfo)
	CONST(GHOST_kStandardCursorDestroy)
	CONST(GHOST_kStandardCursorHelp)
	CONST(GHOST_kStandardCursorCycle)
	CONST(GHOST_kStandardCursorSpray)
	CONST(GHOST_kStandardCursorWait)
	CONST(GHOST_kStandardCursorText)
	CONST(GHOST_kStandardCursorCrosshair)
	CONST(GHOST_kStandardCursorUpDown)
	CONST(GHOST_kStandardCursorLeftRight)
	CONST(GHOST_kStandardCursorTopSide)
	CONST(GHOST_kStandardCursorBottomSide)
	CONST(GHOST_kStandardCursorLeftSide)
	CONST(GHOST_kStandardCursorRightSide)
	CONST(GHOST_kStandardCursorTopLeftCorner)
	CONST(GHOST_kStandardCursorTopRightCorner)
	CONST(GHOST_kStandardCursorBottomRightCorner)
	CONST(GHOST_kStandardCursorBottomLeftCorner)
	CONST(GHOST_kStandardCursorCustom)
	CONST(GHOST_kStandardCursorNumCursors)
	CONST(GHOST_kStandardCursorPencil)

	/** ghost key values **/
	CONST(GHOST_kKeyUnknown)
	CONST(GHOST_kKeyBackSpace)
	CONST(GHOST_kKeyTab)
	CONST(GHOST_kKeyLinefeed)
	CONST(GHOST_kKeyClear)
	CONST(GHOST_kKeyEnter)

	CONST(GHOST_kKeyEsc)
	CONST(GHOST_kKeySpace)
	CONST(GHOST_kKeyQuote)
	CONST(GHOST_kKeyComma)
	CONST(GHOST_kKeyMinus)
	CONST(GHOST_kKeyPeriod)
	CONST(GHOST_kKeySlash)

	// Number keys
	CONST(GHOST_kKey0)
	CONST(GHOST_kKey1)
	CONST(GHOST_kKey2)
	CONST(GHOST_kKey3)
	CONST(GHOST_kKey4)
	CONST(GHOST_kKey5)
	CONST(GHOST_kKey6)
	CONST(GHOST_kKey7)
	CONST(GHOST_kKey8)
	CONST(GHOST_kKey9)

	CONST(GHOST_kKeySemicolon)
	CONST(GHOST_kKeyEqual)

	// Character keys
	CONST(GHOST_kKeyA)
	CONST(GHOST_kKeyB)
	CONST(GHOST_kKeyC)
	CONST(GHOST_kKeyD)
	CONST(GHOST_kKeyE)
	CONST(GHOST_kKeyF)
	CONST(GHOST_kKeyG)
	CONST(GHOST_kKeyH)
	CONST(GHOST_kKeyI)
	CONST(GHOST_kKeyJ)
	CONST(GHOST_kKeyK)
	CONST(GHOST_kKeyL)
	CONST(GHOST_kKeyM)
	CONST(GHOST_kKeyN)
	CONST(GHOST_kKeyO)
	CONST(GHOST_kKeyP)
	CONST(GHOST_kKeyQ)
	CONST(GHOST_kKeyR)
	CONST(GHOST_kKeyS)
	CONST(GHOST_kKeyT)
	CONST(GHOST_kKeyU)
	CONST(GHOST_kKeyV)
	CONST(GHOST_kKeyW)
	CONST(GHOST_kKeyX)
	CONST(GHOST_kKeyY)
	CONST(GHOST_kKeyZ)

	CONST(GHOST_kKeyLeftBracket)
	CONST(GHOST_kKeyRightBracket)
	CONST(GHOST_kKeyBackslash)
	CONST(GHOST_kKeyAccentGrave)

	CONST(GHOST_kKeyLeftShift)
	CONST(GHOST_kKeyRightShift)
	CONST(GHOST_kKeyLeftControl)
	CONST(GHOST_kKeyRightControl)
	CONST(GHOST_kKeyLeftAlt)
	CONST(GHOST_kKeyRightAlt)
	CONST(GHOST_kKeyCommand)
	CONST(GHOST_kKeyGrLess)

	CONST(GHOST_kKeyCapsLock)
	CONST(GHOST_kKeyNumLock)
	CONST(GHOST_kKeyScrollLock)

	CONST(GHOST_kKeyLeftArrow)
	CONST(GHOST_kKeyRightArrow)
	CONST(GHOST_kKeyUpArrow)
	CONST(GHOST_kKeyDownArrow)

	CONST(GHOST_kKeyPrintScreen)
	CONST(GHOST_kKeyPause)

	CONST(GHOST_kKeyInsert)
	CONST(GHOST_kKeyDelete)
	CONST(GHOST_kKeyHome)
	CONST(GHOST_kKeyEnd)
	CONST(GHOST_kKeyUpPage)
	CONST(GHOST_kKeyDownPage)

	// Numpad keys
	CONST(GHOST_kKeyNumpad0)
	CONST(GHOST_kKeyNumpad1)
	CONST(GHOST_kKeyNumpad2)
	CONST(GHOST_kKeyNumpad3)
	CONST(GHOST_kKeyNumpad4)
	CONST(GHOST_kKeyNumpad5)
	CONST(GHOST_kKeyNumpad6)
	CONST(GHOST_kKeyNumpad7)
	CONST(GHOST_kKeyNumpad8)
	CONST(GHOST_kKeyNumpad9)
	CONST(GHOST_kKeyNumpadPeriod)
	CONST(GHOST_kKeyNumpadEnter)
	CONST(GHOST_kKeyNumpadPlus)
	CONST(GHOST_kKeyNumpadMinus)
	CONST(GHOST_kKeyNumpadAsterisk)
	CONST(GHOST_kKeyNumpadSlash)

	// FUNCction keys
	CONST(GHOST_kKeyF1)
	CONST(GHOST_kKeyF2)
	CONST(GHOST_kKeyF3)
	CONST(GHOST_kKeyF4)
	CONST(GHOST_kKeyF5)
	CONST(GHOST_kKeyF6)
	CONST(GHOST_kKeyF7)
	CONST(GHOST_kKeyF8)
	CONST(GHOST_kKeyF9)
	CONST(GHOST_kKeyF10)
	CONST(GHOST_kKeyF11)
	CONST(GHOST_kKeyF12)
	CONST(GHOST_kKeyF13)
	CONST(GHOST_kKeyF14)
	CONST(GHOST_kKeyF15)
	CONST(GHOST_kKeyF16)
	CONST(GHOST_kKeyF17)
	CONST(GHOST_kKeyF18)
	CONST(GHOST_kKeyF19)
	CONST(GHOST_kKeyF20)
	CONST(GHOST_kKeyF21)
	CONST(GHOST_kKeyF22)
	CONST(GHOST_kKeyF23)
	CONST(GHOST_kKeyF24)

	FUNC(GHOST_CreateSystem)
	FUNC(GHOST_DisposeSystem)

	FUNC(GHOST_CreateEventConsumer)
	FUNC(GHOST_DisposeEventConsumer)
	FUNC(GHOST_GetMilliSeconds)
	FUNC(GHOST_InstallTimer)
	FUNC(GHOST_RemoveTimer)
	FUNC(GHOST_GetTimerTaskUserData)
	FUNC(GHOST_SetTimerTaskUserData)

	FUNC(GHOST_AddEventConsumer)
	FUNC(GHOST_ProcessEvents)
	FUNC(GHOST_DispatchEvents)

	FUNC(GHOST_GetNumDisplays)
	FUNC(GHOST_GetMainDisplayDimensions)
	FUNC(GHOST_CreateWindow)
	FUNC(GHOST_DisposeWindow)
	FUNC(GHOST_GetWindowUserData)
	FUNC(GHOST_SetWindowUserData)
	FUNC(GHOST_ValidWindow)
	FUNC(GHOST_InvalidateWindow)
	FUNC(GHOST_BeginFullScreen)
	FUNC(GHOST_EndFullScreen)
	FUNC(GHOST_GetFullScreen)

	FUNC(GHOST_GetEventType)
	FUNC(GHOST_GetEventWindow)
	FUNC(GHOST_GetModifierKeyState)
	FUNC(GHOST_GetButtonState)
	FUNC(GHOST_GetEventTime)
	FUNC(GHOST_GetEventData)
	/* TODO:
		 FUNC(GHOST_GetTimerProc)
		 FUNC(GHOST_SetTimerProc)
		 */
	FUNC(GHOST_GetValid)
	FUNC(GHOST_GetDrawingContextType)
	FUNC(GHOST_SetDrawingContextType)
	FUNC(GHOST_SetTitle)
	FUNC(GHOST_GetTitle)

	FUNC(GHOST_ActivateWindowDrawingContext)
	FUNC(GHOST_SwapWindowBuffers)

	FUNC(GHOST_GetClientBounds)
	FUNC(GHOST_GetWidthRectangle)
	FUNC(GHOST_GetHeightRectangle)
	FUNC(GHOST_DisposeRectangle)
	FUNC(GHOST_GetWindowBounds)
	FUNC(GHOST_SetClientWidth)
	FUNC(GHOST_SetClientHeight)
	FUNC(GHOST_SetClientSize)
	FUNC(GHOST_ScreenToClient)
	FUNC(GHOST_ClientToScreen)
	FUNC(GHOST_GetWindowState)
	FUNC(GHOST_SetWindowState)
	FUNC(GHOST_SetWindowOrder)
	FUNC(GHOST_GetRectangle)
	FUNC(GHOST_SetRectangle)
	FUNC(GHOST_IsEmptyRectangle)
	FUNC(GHOST_IsValidRectangle)
	FUNC(GHOST_InsetRectangle)
	FUNC(GHOST_UnionRectangle)
	FUNC(GHOST_UnionPointRectangle)
	FUNC(GHOST_IsInsideRectangle)
	FUNC(GHOST_GetRectangleVisibility)
	FUNC(GHOST_SetCenterRectangle)
	FUNC(GHOST_SetRectangleCenter)
	FUNC(GHOST_ClipRectangle)

	FUNC(GHOST_SetCursorShape)
	FUNC(GHOST_GetCursorShape)
	/* TODO:
		 FUNC(GHOST_SetCustomCursorShape)
		 FUNC(GHOST_SetCustomCursorShapeEx)
		 */
	FUNC(GHOST_GetCursorVisibility)
	FUNC(GHOST_SetCursorVisibility)
	FUNC(GHOST_GetCursorPosition)
	FUNC(GHOST_SetCursorPosition)

}

LUALIB_API int luaopen_luaghost(lua_State * L)
{
	register_ghost(L);

	/* local luaghost = {} */
	lua_newtable(L);

	/* luaghost.VERSION = version */
	lua_pushstring(L, "VERSION");
	lua_pushstring(L, VERSION);
	lua_settable(L, -3);

	/* luaghost.window = { } */
	lua_pushstring(L, "window");
	lua_newtable(L);
	lua_settable(L, -3);

	/* local timer = { } */
	lua_pushstring(L, "timer");
	lua_newtable(L);

	/* timer.next_free = -1 */
	lua_pushstring(L, "next_free");
	lua_pushnumber(L, -1);
	lua_settable(L, -3);

	/* luaghost.timer = timer */
	lua_settable(L, -3);

	/***************/
	/* local consumer = { } */
	lua_pushstring(L, "consumer");
	lua_newtable(L);

	/* consumer.next_free = -1 */
	lua_pushstring(L, "next_free");
	lua_pushnumber(L, -1);
	lua_settable(L, -3);

	/* luaghost.consumer = consumer */
	lua_settable(L, -3);
	/***************/

	/* _G.luaghost = luaghost */
	lua_pushstring(L, LUA_LUAGHOSTNAME);
	lua_pushvalue(L, -2);
	lua_settable(L, LUA_GLOBALSINDEX);

	ref_L = L;

	return 1;
}

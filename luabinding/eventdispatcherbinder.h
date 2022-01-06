#ifndef EVENTDISPATCHERBINDER_H
#define EVENTDISPATCHERBINDER_H

#include "binder.h"

class EventDispatcherBinder
{
public:
	EventDispatcherBinder(lua_State* L);

private:
	static int create(lua_State* L);
	static int destruct(void *p);

	static int addEventListener(lua_State* L);
	static int dispatchEvent(lua_State* L);
	static int removeEventListener(lua_State* L);
	static int hasEventListener(lua_State* L);
	static int removeAllListeners(lua_State* L);
};

#endif


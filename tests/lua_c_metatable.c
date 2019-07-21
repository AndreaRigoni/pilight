/*
	Copyright (C) 2013 - 2016 CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../libs/pilight/core/log.h"
#include "../libs/pilight/core/CuTest.h"
#include "../libs/pilight/core/pilight.h"
#include "../libs/pilight/core/common.h"
#include "../libs/pilight/lua_c/lua.h"
#include "../libs/pilight/lua_c/table.h"
#include "../libs/pilight/lua_c/lualibrary.h"

static CuTest *gtc = NULL;
static int run = 0;
static int test = 0;

static void close_cb(uv_handle_t *handle) {
	FREE(handle);
}

static void walk_cb(uv_handle_t *handle, void *arg) {
	if(!uv_is_closing(handle)) {
		uv_close(handle, close_cb);
	}
}

static int plua_print(lua_State* L) {
	switch(test) {
		case 0: {
			switch(run) {
				case 0:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "b", lua_tostring(L, -1));
					run++;
				break;
				case 1:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "c", lua_tostring(L, -1));
					run++;
				break;
				case 2:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 3:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "d", lua_tostring(L, -1));
					run++;
				break;
				case 4:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "b", lua_tostring(L, -1));
					run++;
				break;
				case 5:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "c", lua_tostring(L, -1));
					run++;
				break;
				case 6:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 7:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "d", lua_tostring(L, -1));
					run++;
				break;
				case 8:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
				case 9:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 2, lua_tonumber(L, -1));
					run++;
				break;
				case 10:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 11:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 3, lua_tonumber(L, -1));
					run++;
				break;
				case 12:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 2, lua_tonumber(L, -1));
					run++;
				break;
				case 13:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 4, lua_tonumber(L, -1));
					run++;
				break;
				case 14:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 15:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 5, lua_tonumber(L, -1));
					run++;
				break;
				case 16:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 17:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 18:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 3, lua_tonumber(L, -1));
					run++;
				break;
				case 19:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 20:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "d", lua_tostring(L, -1));
					run++;
				break;
				case 21:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 5, lua_tonumber(L, -1));
					run++;
				break;
				case 22:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "c", lua_tostring(L, -1));
					run++;
				break;
				case 23:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 2, lua_tonumber(L, -1));
					run++;
				break;
				case 24:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
				case 25:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 2, lua_tonumber(L, -1));
					run++;
				break;
				case 26:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 2, lua_tonumber(L, -1));
					run++;
				break;
				case 27:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
				case 28:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
				case 29:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
				case 30:
					CuAssertIntEquals(gtc, LUA_TBOOLEAN, lua_type(L, -1));
					CuAssertIntEquals(gtc, 0, lua_toboolean(L, -1));
					run++;
				break;
				case 31:
					CuAssertIntEquals(gtc, LUA_TBOOLEAN, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_toboolean(L, -1));
					run++;
				break;
				case 32:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
				case 33:
					CuAssertIntEquals(gtc, LUA_TBOOLEAN, lua_type(L, -1));
					CuAssertIntEquals(gtc, 0, lua_toboolean(L, -1));
					run++;
				break;
				case 34:
					CuAssertIntEquals(gtc, LUA_TBOOLEAN, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_toboolean(L, -1));
					run++;
				break;
				case 35:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
				case 36:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
				case 37:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 10, lua_tonumber(L, -1));
					run++;
				break;
				case 38:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 10, lua_tonumber(L, -1));
					run++;
				break;
				case 39:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 11, lua_tonumber(L, -1));
					run++;
				break;
				case 40:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 11, lua_tonumber(L, -1));
					run++;
				break;
				case 41:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 10, lua_tonumber(L, -1));
					run++;
				break;
				case 42:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 43:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 10, lua_tonumber(L, -1));
					run++;
				break;
				case 44:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 12, lua_tonumber(L, -1));
					run++;
				break;
				case 45:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 13, lua_tonumber(L, -1));
					run++;
				break;
				case 46:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 12, lua_tonumber(L, -1));
					run++;
				break;
				case 47:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 10, lua_tonumber(L, -1));
					run++;
				break;
				case 48:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 13, lua_tonumber(L, -1));
					run++;
				break;
				case 49:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 10, lua_tonumber(L, -1));
					run++;
				break;
				case 50:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 12, lua_tonumber(L, -1));
					run++;
				break;
			}
		} break;
		case 1: {
			switch(run) {
				case 0:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "a", lua_tostring(L, -1));
					run++;
				break;
				case 1:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 2:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 2, lua_tonumber(L, -1));
					run++;
				break;
				case 3:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 4:
					CuAssertIntEquals(gtc, LUA_TBOOLEAN, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, (int)lua_toboolean(L, -1));
					run++;
				break;
				case 5:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 6:
					CuAssertIntEquals(gtc, LUA_TTABLE, lua_type(L, -1));
					run++;
				break;
				case 7:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "a", lua_tostring(L, -1));
					run++;
				break;
				case 8:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "b", lua_tostring(L, -1));
					run++;
				break;
				case 9:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "c", lua_tostring(L, -1));
					run++;
				break;
				case 10:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "d", lua_tostring(L, -1));
					run++;
				break;
				case 11:
					CuAssertIntEquals(gtc, LUA_TNIL, lua_type(L, -1));
					run++;
				break;
				case 12:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
				case 13:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "a", lua_tostring(L, -1));
					run++;
				break;
				case 14:
					CuAssertIntEquals(gtc, LUA_TBOOLEAN, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, (int)lua_toboolean(L, -1));
					run++;
				break;
				case 15:
					CuAssertIntEquals(gtc, LUA_TSTRING, lua_type(L, -1));
					CuAssertStrEquals(gtc, "a", lua_tostring(L, -1));
					run++;
				break;
				case 16:
					CuAssertIntEquals(gtc, LUA_TBOOLEAN, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, (int)lua_toboolean(L, -1));
					run++;
				break;
				case 17:
					CuAssertIntEquals(gtc, LUA_TNUMBER, lua_type(L, -1));
					CuAssertIntEquals(gtc, 1, lua_tonumber(L, -1));
					run++;
				break;
			}
		} break;
	}
	return 1;
}

static void test_lua_c_metatable(CuTest *tc) {
	printf("[ %-48s ]\n", __FUNCTION__);
	fflush(stdout);

	memtrack();

	gtc = tc;

	run = 0;
	test = 0;

	struct lua_state_t *state = NULL;

	plua_init();
	plua_override_global("print", plua_print);
	plua_pause_coverage(1);

	state = plua_get_free_state();
	CuAssertPtrNotNull(gtc, state);
	CuAssertIntEquals(gtc, 0, luaL_dostring(state->L, " \
		local thread = pilight.async.thread(); \
		local data = thread.getUserdata(); \
		data['a'] = 'b'; \
		data['b'] = 'c'; \
		print(data['a']); \
		print(data['b']); \
		data['a'] = nil; \
		print(data['a']); \
		data['b'] = 'd'; \
		print(data['b']); \
		data[1] = 'b'; \
		data[2] = 'c'; \
		print(data[1]); \
		print(data[2]); \
		data[1] = nil; \
		print(data[1]); \
		data[2] = 'd'; \
		print(data[2]); \
		data['b'] = 1; \
		data['c'] = 2; \
		print(data['b']); \
		print(data['c']); \
		data['b'] = nil; \
		print(data['b']); \
		data['c'] = 3; \
		print(data['c']); \
		data[1] = 2; \
		data[3] = 4; \
		print(data[1]); \
		print(data[3]); \
		data[1] = nil; \
		print(data[1]); \
		data[3] = 5; \
		print(data[3]); \
		local thread1 = pilight.async.thread(); \
		local foo = 1;\
		thread1.setUserdata(data()); \
		local data1 = thread1.getUserdata(); \
		print(data1['a']); \
		print(data1['b']); \
		print(data1['c']); \
		print(data1[1]); \
		print(data1[2]); \
		print(data1[3]); \
	"));

	/*
	 * After the print are a few tests that
	 * check if old tables are properly freed
	 */
	CuAssertIntEquals(gtc, 0, luaL_dostring(state->L, " \
		local thread = pilight.async.thread(); \
		local data = thread.getUserdata(); \
		local a = {}; \
		a['b'] = {}; \
		a['b'][1] = 'c'; \
		a['b']['d'] = 2; \
		thread.setUserdata(a); \
		print(data['b'][1]); \
		print(data['b']['d']); \
		data['b'] = nil; \
		data['b'] = {}; \
		data['b'] = 'a'; \
		data['b'] = {}; \
		data['b'] = {}; \
		data['b'] = 1; \
	"));

	CuAssertIntEquals(gtc, 0, luaL_dostring(state->L, " \
		local z = pilight.table(); \
		local y = pilight.table(); \
		local x = pilight.table(); \
		z['a'] = 1; \
		z['b'] = 2; \
		y['c'] = z; \
		x['d'] = y; \
		print(x['d']['c']['a']); \
		print(x['d']['c']['b']); \
		print(z.len()); \
		print(y.len()); \
		print(x.len()); \
	"));

	CuAssertIntEquals(gtc, 0, luaL_dostring(state->L, " \
		local thread = pilight.async.thread(); \
		local data = thread.getUserdata(); \
		local z = pilight.table(); \
		z['a'] = 1; \
		z['b'] = false; \
		z['c'] = true; \
		print(z['a']); \
		print(z['b']); \
		print(z['c']); \
		thread.setUserdata(z());\
		print(data['a']); \
		print(data['b']); \
		print(data['c']); \
	"));

	CuAssertIntEquals(gtc, 0, luaL_dostring(state->L, " \
		local thread = pilight.async.thread(); \
		local data = thread.getUserdata(); \
		local z = pilight.table(); \
		z['a'] = pilight.table(); \
		z['a']['b'] = 1; \
		print(z['a']['b']); \
		thread.setUserdata(z['a']());\
		print(data['b']); \
	"));

	/*
	 * Test for invalid (boolean) index
	 */
	CuAssertIntEquals(gtc, 1, luaL_dostring(state->L, " \
		local z = pilight.table(); \
		z[false] = 1; \
	"));

	/*
	 * Test proper free of empty table in
	 * plua_metatable_free
	 */
	CuAssertIntEquals(gtc, 0, luaL_dostring(state->L, " \
		local thread = pilight.async.thread(); \
		local thread1 = pilight.async.thread(); \
		thread.setUserdata(thread1.getUserdata()()); \
	"));

	CuAssertIntEquals(gtc, 0, luaL_dostring(state->L, " \
		local data = pilight.table(); \
		data.push(10); \
		print(data[1]); \
		data.push(11); \
		print(data[1]); \
		print(data[2]); \
		print(data.pop()); \
		print(data.pop()); \
		print(data[1]); \
		data.push(10); \
		print(data[1]); \
		data.shift(12); \
		print(data[1]); \
		data.shift(13); \
		print(data[1]); \
		print(data[2]); \
		print(data[3]); \
		print(data.unshift()); \
		print(data.pop()); \
		print(data[1]); \
	"));

	lua_remove(state->L, -1);
	CuAssertIntEquals(gtc, 1, luaL_dostring(state->L, " \
		local data = pilight.table(); \
		data.push(10); \
		data['a'] = 1; \
		data.push(11); \
	"));

	lua_remove(state->L, -1);

	CuAssertIntEquals(gtc, 1, luaL_dostring(state->L, " \
		local data = pilight.table(); \
		data.push(); \
	"));

	lua_remove(state->L, -1);

	CuAssertIntEquals(gtc, 1, luaL_dostring(state->L, " \
		local data = pilight.table(); \
		data.pop(1); \
	"));

	lua_remove(state->L, -1);

	CuAssertIntEquals(gtc, 1, luaL_dostring(state->L, " \
		local data = pilight.table(); \
		data.unshift(1); \
	"));

	lua_remove(state->L, -1);

	CuAssertIntEquals(gtc, 1, luaL_dostring(state->L, " \
		local data = pilight.table(); \
		data.shift(); \
	"));

	lua_remove(state->L, -1);
	plua_clear_state(state);

	uv_run(uv_default_loop(), UV_RUN_NOWAIT);
	uv_walk(uv_default_loop(), walk_cb, NULL);
	uv_run(uv_default_loop(), UV_RUN_NOWAIT);

	while(uv_loop_close(uv_default_loop()) == UV_EBUSY) {
		uv_run(uv_default_loop(), UV_RUN_NOWAIT);
	}

	plua_pause_coverage(0);
	plua_gc();

	CuAssertIntEquals(tc, 51, run);
	CuAssertIntEquals(tc, 0, xfree());
}

static void test_c_lua_metatable(CuTest *tc) {
	printf("[ %-48s ]\n", __FUNCTION__);
	fflush(stdout);

	memtrack();

	gtc = tc;

	run = 0;
	test = 1;

	struct lua_state_t *state = NULL;

	plua_init();
	plua_override_global("print", plua_print);
	plua_pause_coverage(1);

	state = plua_get_free_state();
	CuAssertPtrNotNull(gtc, state);

	{
		struct plua_metatable_t *table = NULL;
		plua_metatable_init(&table);
		CuAssertPtrNotNull(tc, table);

		plua_metatable_set_string(table, "a", "a");
		luaL_loadstring(state->L, " \
			print(_table['a']);\
			print(_table['b']);\
		");

		push_plua_metatable(state->L, table);
		lua_setglobal(state->L, "_table");
		plua_pcall(state->L, __FILE__, 0, LUA_MULTRET);
		plua_metatable_free(table);
	}

	{
		struct plua_metatable_t *table = NULL;
		plua_metatable_init(&table);
		CuAssertPtrNotNull(tc, table);

		plua_metatable_set_number(table, "1", 2);
		luaL_loadstring(state->L, " \
			print(_table[1]);\
			print(_table[2]);\
		");

		push_plua_metatable(state->L, table);
		lua_setglobal(state->L, "_table");
		plua_pcall(state->L, __FILE__, 0, LUA_MULTRET);
		plua_metatable_free(table);
	}

	{
		struct plua_metatable_t *table = NULL;
		plua_metatable_init(&table);
		CuAssertPtrNotNull(tc, table);

		plua_metatable_set_boolean(table, "a", 1);
		luaL_loadstring(state->L, " \
			print(_table['a']);\
			print(_table['b']);\
		");

		push_plua_metatable(state->L, table);
		lua_setglobal(state->L, "_table");
		plua_pcall(state->L, __FILE__, 0, LUA_MULTRET);
		plua_metatable_free(table);
	}

	{
		struct plua_metatable_t *table = NULL;
		plua_metatable_init(&table);
		CuAssertPtrNotNull(tc, table);

		plua_metatable_set_string(table, "a.1.c.1.e", "a");
		plua_metatable_set_string(table, "a.1.c.2.e", "b");
		plua_metatable_set_string(table, "a.2.c.1.e", "c");
		plua_metatable_set_string(table, "a.2.c.2.e", "d");
		luaL_loadstring(state->L, " \
			print(_table['a']);\
			print(_table['a'][1]['c'][1]['e']);\
			print(_table['a'][1]['c'][2]['e']);\
			print(_table['a'][2]['c'][1]['e']);\
			print(_table['a'][2]['c'][2]['e']);\
			print(_table['b']);\
		");

		push_plua_metatable(state->L, table);
		lua_setglobal(state->L, "_table");
		plua_pcall(state->L, __FILE__, 0, LUA_MULTRET);
		plua_metatable_free(table);
	}

	{
		struct plua_metatable_t *table = NULL;
		plua_metatable_init(&table);
		CuAssertPtrNotNull(tc, table);

		plua_metatable_set_string(table, "a", "a");
		plua_metatable_set_number(table, "a", 1);

		plua_metatable_set_number(table, "b", 1);
		plua_metatable_set_string(table, "b", "a");

		plua_metatable_set_string(table, "c", "a");
		plua_metatable_set_boolean(table, "c", 1);

		plua_metatable_set_boolean(table, "d", 1);
		plua_metatable_set_string(table, "d", "a");

		plua_metatable_set_number(table, "e", 1);
		plua_metatable_set_boolean(table, "e", 1);

		plua_metatable_set_boolean(table, "f", 1);
		plua_metatable_set_number(table, "f", 1);

		luaL_loadstring(state->L, " \
			print(_table['a']);\
			print(_table['b']);\
			print(_table['c']);\
			print(_table['d']);\
			print(_table['e']);\
			print(_table['f']);\
		");

		push_plua_metatable(state->L, table);
		lua_setglobal(state->L, "_table");
		plua_pcall(state->L, __FILE__, 0, LUA_MULTRET);
		plua_metatable_free(table);
	}

	{
		luaL_loadstring(state->L, " \
			local table = pilight.table(); \
			table[1] = 'a'; \
			table['c'] = 1; \
			table['b'] = true; \
			table['a'] = {}; \
			table['a'][1] = {}; \
			table['a'][2] = {}; \
			table['a'][1]['c'] = {}; \
			table['a'][2]['c'] = {}; \
			table['a'][1]['c'][1] = {}; \
			table['a'][1]['c'][2] = {}; \
			table['a'][2]['c'][1] = {}; \
			table['a'][2]['c'][2] = {}; \
			table['a'][1]['c'][1]['e'] = 'a'; \
			table['a'][1]['c'][2]['e'] = 'b'; \
			table['a'][2]['c'][1]['e'] = 'c'; \
			table['a'][2]['c'][2]['e'] = 'd'; \
			return table(); \
		");

		plua_pcall(state->L, __FILE__, 0, 1);

		struct plua_metatable_t *table = NULL, *cpy = NULL;

		CuAssertIntEquals(tc, LUA_TLIGHTUSERDATA, lua_type(state->L, -1));

		cpy = (void *)lua_topointer(state->L, -1);
		lua_remove(state->L, -1);
		plua_metatable_clone(&cpy, &table);

		char *str = NULL;
		double num = 0.0;
		int _bool = 0;

		CuAssertIntEquals(tc, -1, plua_metatable_get_string(table, "0", &str));
		CuAssertIntEquals(tc, -1, plua_metatable_get_string(table, "foo", &str));
		CuAssertIntEquals(tc, -1, plua_metatable_get_string(table, "c", &str));
		CuAssertIntEquals(tc, -1, plua_metatable_get_string(table, "b", &str));
		CuAssertIntEquals(tc, -1, plua_metatable_get_number(table, "0", &num));
		CuAssertIntEquals(tc, -1, plua_metatable_get_number(table, "1", &num));
		CuAssertIntEquals(tc, -1, plua_metatable_get_number(table, "a", &num));
		CuAssertIntEquals(tc, -1, plua_metatable_get_boolean(table, "1", &_bool));
		CuAssertIntEquals(tc, -1, plua_metatable_get_boolean(table, "c", &_bool));
		CuAssertIntEquals(tc, -1, plua_metatable_get_boolean(table, "a", &_bool));

		CuAssertIntEquals(tc, 0, plua_metatable_get_string(table, "1", &str));
		CuAssertStrEquals(tc, "a", str);

		CuAssertIntEquals(tc, 0, plua_metatable_get_number(table, "c", &num));
		CuAssertIntEquals(tc, 1, num);

		CuAssertIntEquals(tc, 0, plua_metatable_get_boolean(table, "b", &_bool));
		CuAssertIntEquals(tc, 1, _bool);

		CuAssertIntEquals(tc, 0, plua_metatable_get_string(table, "a.1.c.1.e", &str));
		CuAssertStrEquals(tc, "a", str);

		CuAssertIntEquals(tc, 0, plua_metatable_get_string(table, "a.1.c.2.e", &str));
		CuAssertStrEquals(tc, "b", str);

		CuAssertIntEquals(tc, 0, plua_metatable_get_string(table, "a.2.c.1.e", &str));
		CuAssertStrEquals(tc, "c", str);

		CuAssertIntEquals(tc, 0, plua_metatable_get_string(table, "a.2.c.2.e", &str));
		CuAssertStrEquals(tc, "d", str);

		plua_metatable_free(table);
	}

	plua_clear_state(state);

	uv_run(uv_default_loop(), UV_RUN_NOWAIT);
	uv_walk(uv_default_loop(), walk_cb, NULL);
	uv_run(uv_default_loop(), UV_RUN_NOWAIT);

	while(uv_loop_close(uv_default_loop()) == UV_EBUSY) {
		uv_run(uv_default_loop(), UV_RUN_NOWAIT);
	}

	plua_pause_coverage(0);
	plua_gc();

	CuAssertIntEquals(tc, 18, run);
	CuAssertIntEquals(tc, 0, xfree());
}

CuSuite *suite_lua_c_metatable(void) {
	CuSuite *suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, test_lua_c_metatable);
	SUITE_ADD_TEST(suite, test_c_lua_metatable);

	return suite;
}

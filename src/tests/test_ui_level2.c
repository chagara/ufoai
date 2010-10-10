/**
 * @file test_ui.c
 * @brief Test cases for code below client/ui/ when the global UI "engine" is started
 */

/*
Copyright (C) 2002-2010 UFO: Alien Invasion.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "CUnit/Basic.h"
#include "test_shared.h"
#include "test_ui_level2.h"
#include "../client/ui/ui_nodes.h"
#include "../client/ui/ui_main.h"

/**
 * The suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
static int UFO_InitSuiteUILevel2 (void)
{
	TEST_Init();
	/** @todo why must we call Cmd_Init + Cbuf_Init?, the last one should be useless */
	Cbuf_Init();
	UI_Init();
	return 0;
}

/** @todo move it somewhere */
static void TEST_ParseScript (const char* scriptName)
{
	const char *type, *name, *text;

	/* parse ui node script */
	Com_Printf("Load \"%s\": %i script file(s)\n", scriptName, FS_BuildFileList(scriptName));
	FS_NextScriptHeader(NULL, NULL, NULL);
	text = NULL;
	while ((type = FS_NextScriptHeader(scriptName, &name, &text)) != NULL)
		CL_ParseClientData(type, name, &text);
}

/**
 * The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
static int UFO_CleanSuiteUILevel2 (void)
{
	UI_Shutdown();
	TEST_Shutdown();
	return 0;
}

/**
 * @brief after execution of a unittest window, it analyse color of
 * normalized indicator, and create asserts. Both Fail/Pass asserts is
 * usefull to count number.
 */
static void UFO_AnalyseTestWindow (const char* windowName)
{
	const uiBehaviour_t *stringBehaviour = UI_GetNodeBehaviour("string");
	const uiNode_t *node;
	const uiNode_t *window = UI_GetWindow(windowName);
	CU_ASSERT_FATAL(window != NULL);
	CU_ASSERT_FATAL(stringBehaviour != NULL);

	for (node = window->firstChild; node != NULL; node = node->next) {
		qboolean isGreen;
		qboolean isRed;
		/* skip non "string" nodes */
		if (node->behaviour != stringBehaviour)
			continue;
		if (node->invis)
			continue;
		/* skip nodes without "test" prefix */
		if (strncmp(node->name, "test", 4) != 0)
			continue;

		isGreen = node->color[0] < 0.1 && node->color[1] > 0.9 && node->color[2] < 0.1;
		isRed = node->color[0] > 0.9 && node->color[1] < 0.1 && node->color[2] < 0.1;

		if (isGreen) {
			/** @note Useful to count number of tests */
			CU_ASSERT_TRUE(qtrue);
		} else if (isRed) {
			const char *message = va("%s.%s failed.", windowName, node->name);
			/*Com_Printf("Error: %s\n", message);*/
			/*CU_FAIL(message);*/
			CU_assertImplementation(CU_FALSE, 0, va("CU_FAIL(%s)", message), va("base/ufos/uitest/%s.ufo", windowName), "", CU_FALSE);
		} else {
			const char *message = va("%s.%s have an unknown status.", windowName, node->name);
			/*Com_Printf("Warning: %s\n", message);*/
			/*CU_FAIL(message);*/
			CU_assertImplementation(CU_FALSE, 0, va("CU_FAIL(%s)", message), va("base/ufos/uitest/%s.ufo", windowName), "", CU_FALSE);
		}
	}
}

/**
 * @brief Parse and execute a test windows
 */
static void UFO_ExecuteTestWindow (const char* windowName)
{
	int i;

	/* look and feed */
	Com_Printf("\n");

	TEST_ParseScript(va("ufos/uitest/%s.ufo", windowName));
	/* @todo catch parse error */

	Cmd_ExecuteString(va("ui_push %s", windowName));

	/* while the execution buffer is not empty */
	for (i = 0; i < 20; i++) {
		Cbuf_Execute();
	}

	UFO_AnalyseTestWindow(windowName);

	/* look and feed */
	Com_Printf("  ---> ");
}

/**
 * @brief test behaviour name
 */
static void testKnownBehaviours (void)
{
	uiBehaviour_t *behaviour;

	behaviour = UI_GetNodeBehaviour("button");
	CU_ASSERT_FATAL(behaviour != NULL);
	CU_ASSERT_STRING_EQUAL(behaviour->name, "button");

	/* first one */
	behaviour = UI_GetNodeBehaviour("");
	CU_ASSERT(behaviour != NULL);

	/* last one */
	behaviour = UI_GetNodeBehaviour("zone");
	CU_ASSERT_FATAL(behaviour != NULL);
	CU_ASSERT_STRING_EQUAL(behaviour->name, "zone");

	/* unknown */
	behaviour = UI_GetNodeBehaviour("dahu");
	CU_ASSERT(behaviour == NULL);
}

/**
 * @brief test actions
 */
static void testActions (void)
{
	UFO_ExecuteTestWindow("test_action");
}

/**
 * @brief test actions
 */
static void testActions2 (void)
{
	UFO_ExecuteTestWindow("test_action2");
}

/**
 * @brief test conditions
 */
static void testConditions (void)
{
	UFO_ExecuteTestWindow("test_condition");
}

/**
 * @brief test function
 */
static void testFunctions (void)
{
	UFO_ExecuteTestWindow("test_function");
}

/**
 * @brief test action
 */
static void testSetters (void)
{
	UFO_ExecuteTestWindow("test_setter");
}

int UFO_AddUILevel2Tests (void)
{
	/* add a suite to the registry */
	CU_pSuite UISuite = CU_add_suite("UILevel2Tests", UFO_InitSuiteUILevel2, UFO_CleanSuiteUILevel2);

	if (UISuite == NULL)
		return CU_get_error();

	/* add the tests to the suite */
	if (CU_ADD_TEST(UISuite, testKnownBehaviours) == NULL)
		return CU_get_error();
	if (CU_ADD_TEST(UISuite, testActions) == NULL)
		return CU_get_error();
	if (CU_ADD_TEST(UISuite, testActions2) == NULL)
		return CU_get_error();
	if (CU_ADD_TEST(UISuite, testConditions) == NULL)
		return CU_get_error();
	if (CU_ADD_TEST(UISuite, testFunctions) == NULL)
		return CU_get_error();
	if (CU_ADD_TEST(UISuite, testSetters) == NULL)
		return CU_get_error();

	return CUE_SUCCESS;
}

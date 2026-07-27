/*
 * stubs.cpp — definitions for global state shared across the test suite.
 *
 * - The suite registry (_suites[], _suiteCount) must have a single definition
 *   so that static constructors in every translation unit append to the same
 *   array (C++ static init order across TUs is unspecified but within the same
 *   TU it is guaranteed, and all _SuiteRegistrar constructors run before main).
 * - Test state (_suiteName, _testName, _testFailed, _g, _s) is also a single
 *   instance here; all test files share it via extern.
 */

#include "../framework.h"
#include "arduino_stub.h"

// ─── Suite registry ───────────────────────────────────────────────────────────
_SuiteEntry _suites[256];
int         _suiteCount = 0;

// ─── Test state ───────────────────────────────────────────────────────────────
const char* _suiteName = "";
const char* _testName  = "";
bool        _testFailed = false;
_Counters   _g;
_Counters   _s;

// ─── Arduino stub instance ────────────────────────────────────────────────────
_SerialStub Serial;

/*
 * runner.cpp — entry point for the native test suite.
 *
 * Defines RUNNER_MAIN so that framework.h generates main().
 * All test files register their suites automatically via static constructors.
 */

#define RUNNER_MAIN
#include "framework.h"

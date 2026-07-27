#pragma once
/*
 * Minimal self-contained test framework.
 *
 * Usage:
 *   SUITE("My module") { ... }        — open a test suite (a function body)
 *   TEST("description") { ... }       — a named test inside a SUITE
 *   ASSERT_EQ(a, b)                   — equality; prints diff on failure
 *   ASSERT_NE(a, b)                   — inequality
 *   ASSERT_TRUE(expr)                 — truthy check
 *   ASSERT_FALSE(expr)                — falsy check
 *   ASSERT_STREQ(s1, s2)              — strcmp == 0
 *   ASSERT_STRNE(s1, s2)              — strcmp != 0
 *   ASSERT_NEAR(a, b, eps)            — float proximity
 *
 * In runner.cpp, define RUNNER_MAIN and include framework.h — it generates
 * main() that calls all registered suites.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

// ─── Suite registry — defined once in stubs.cpp ───────────────────────────────

typedef void (*SuiteFn)();

struct _SuiteEntry {
    const char* name;
    SuiteFn     fn;
};

// Extern declarations: the actual arrays live in stubs.cpp.
extern _SuiteEntry _suites[];
extern int         _suiteCount;

// ─── Per-test state — also defined in stubs.cpp ───────────────────────────────

extern const char* _suiteName;
extern const char* _testName;
extern bool        _testFailed;

struct _Counters {
    int pass = 0;
    int fail = 0;
    int total = 0;
};
extern _Counters _g;   // global counters
extern _Counters _s;   // current-suite counters

// ─── Registration helper ──────────────────────────────────────────────────────

struct _SuiteRegistrar {
    _SuiteRegistrar(const char* name, SuiteFn fn) {
        _suites[_suiteCount++] = {name, fn};
    }
};

// ─── Assertion helpers ─────────────────────────────────────────────────────────

static inline void _fail(const char* file, int line, const char* expr) {
    printf("  FAIL  [%s] %s\n         %s:%d: %s\n",
           _suiteName, _testName, file, line, expr);
    _testFailed = true;
}

#define ASSERT_TRUE(expr) \
    do { if (!(expr)) { _fail(__FILE__, __LINE__, "Expected TRUE: " #expr); } } while(0)

#define ASSERT_FALSE(expr) \
    do { if ((expr))  { _fail(__FILE__, __LINE__, "Expected FALSE: " #expr); } } while(0)

#define ASSERT_EQ(a, b) \
    do { if (!((a) == (b))) { \
        char _msg[256]; \
        snprintf(_msg, sizeof(_msg), #a " == " #b " (got %lld vs %lld)", \
                 (long long)(a), (long long)(b)); \
        _fail(__FILE__, __LINE__, _msg); } } while(0)

#define ASSERT_NE(a, b) \
    do { if ((a) == (b)) { \
        char _msg[256]; \
        snprintf(_msg, sizeof(_msg), #a " != " #b " (both == %lld)", (long long)(a)); \
        _fail(__FILE__, __LINE__, _msg); } } while(0)

#define ASSERT_STREQ(s1, s2) \
    do { if (strcmp((s1), (s2)) != 0) { \
        char _msg[512]; \
        snprintf(_msg, sizeof(_msg), "STREQ failed: \"%s\" != \"%s\"", (s1), (s2)); \
        _fail(__FILE__, __LINE__, _msg); } } while(0)

#define ASSERT_STRNE(s1, s2) \
    do { if (strcmp((s1), (s2)) == 0) { \
        char _msg[512]; \
        snprintf(_msg, sizeof(_msg), "STRNE failed: both == \"%s\"", (s1)); \
        _fail(__FILE__, __LINE__, _msg); } } while(0)

#define ASSERT_NEAR(a, b, eps) \
    do { if (fabs((double)(a) - (double)(b)) > (double)(eps)) { \
        char _msg[256]; \
        snprintf(_msg, sizeof(_msg), "NEAR failed: |%g - %g| > %g", \
                 (double)(a), (double)(b), (double)(eps)); \
        _fail(__FILE__, __LINE__, _msg); } } while(0)

// ─── TEST macro ───────────────────────────────────────────────────────────────

#define TEST(desc) \
    for (bool _once = (_testName = (desc), _testFailed = false, true); \
         _once; \
         _once = false, \
         _s.total++, _g.total++, \
         (_testFailed ? (_s.fail++, _g.fail++) : (_s.pass++, _g.pass++)) \
        )

// ─── SUITE macro ──────────────────────────────────────────────────────────────
// The two-level indirection (_SUITE_CAT + _SUITE_IMPL) is necessary:
//   - SUITE(name) passes __COUNTER__ to _SUITE_IMPL.
//   - _SUITE_IMPL(name, N) receives the already-expanded integer N and
//     delegates to _SUITE_BUILD which does the actual token-pasting.
// Without this extra level, ## suppresses macro expansion, giving
// _suite___COUNTER__ instead of e.g. _suite_3.

#define _SUITE_CAT2(a, b) a##b
#define _SUITE_FN(N)      _SUITE_CAT2(_suite_, N)
#define _SUITE_REG(N)     _SUITE_CAT2(_reg_,   N)

#define _SUITE_BUILD(name, N) \
    static void _SUITE_FN(N)(); \
    static _SuiteRegistrar _SUITE_REG(N)(name, _SUITE_FN(N)); \
    static void _SUITE_FN(N)()

#define SUITE(name) _SUITE_BUILD(name, __COUNTER__)

// ─── Runner (only compiled in runner.cpp) ─────────────────────────────────────

#ifdef RUNNER_MAIN
int main() {
    for (int i = 0; i < _suiteCount; i++) {
        _suiteName = _suites[i].name;
        _s = {};
        printf("\n▸ %s\n", _suiteName);
        _suites[i].fn();
        printf("  %d/%d passed", _s.pass, _s.total);
        if (_s.fail) printf("  (%d FAILED)", _s.fail);
        printf("\n");
    }
    printf("\n%s  Total: %d passed, %d failed out of %d tests\n",
           _g.fail == 0 ? "✓ ALL PASS" : "✗ FAILURES",
           _g.pass, _g.fail, _g.total);
    return _g.fail == 0 ? 0 : 1;
}
#endif

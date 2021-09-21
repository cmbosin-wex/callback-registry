#if !defined(UNITTEST_H)
#define UNITTEST_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

typedef struct {
  const char *name;
  void (*test)(void);
  int failed_assertions;
  int failed_checks;
  char status;
  void *state;
} unittest_test_t;

static int UnittestEvaluate(const char *file, int line, int level,
                            const char *expr, int result, const char *msg);
static void UnittestInit(int argc, char const *argv[]);
static int UnittestRun(unittest_test_t tests[]);

#define UNITTEST_LEVEL_ASSERTION 1
#define UNITTEST_LEVEL_CHECK 2

#define UNITTEST_CHECK(x) \
  UnittestEvaluate(__FILE__, __LINE__, UNITTEST_LEVEL_CHECK, #x, x, NULL)
#define UNITTEST_CHECK_(x, msg) \
  UnittestEvaluate(__FILE__, __LINE__, UNITTEST_LEVEL_CHECK, #x, x, msg)
#define UNITTEST_ASSERT(x)                                                  \
  if (UnittestEvaluate(__FILE__, __LINE__, UNITTEST_LEVEL_ASSERTION, #x, x, \
                       NULL) > 0)                                           \
    return;
#define UNITTEST_ASSERT_(x, msg)                                            \
  if (UnittestEvaluate(__FILE__, __LINE__, UNITTEST_LEVEL_ASSERTION, #x, x, \
                       msg) > 0)                                            \
    return;

#define UNITTEST_TEST(x) void x(void)

#define UNITTEST_TEST_SUITE_SETUP(suite) void UnittestSuiteSetup__##suite(void)

#define UNITTEST_TEST_SUITE_TEARDOWN(suite) \
  void UnittestSuiteTearDown__##suite(void)

#define UNITTEST_TEST_CASE(suite, test)               \
  UNITTEST_TEST(UnittestSuitTestCase__##suite##test); \
  UNITTEST_TEST(UnitTestWrapper__##suite##test) {     \
    (UnittestSuiteSetup__##suite)();                  \
    (UnittestSuitTestCase__##suite##test)();          \
    (UnittestSuiteTearDown__##suite)();               \
  }                                                   \
  UNITTEST_TEST(UnittestSuitTestCase__##suite##test)

#define UNITTEST_TESTS unittest_test_t test_list[]

#define UNITTEST_DECLARE(x) \
  { #x, x, 0, 0, 0, NULL }

#define UNITTEST_DECLARE_TEST_CASE(suite, test) \
  { #suite "::" #test, UnitTestWrapper__##suite##test, 0, 0, 0, NULL }

#define UNITTEST_END \
  { NULL, NULL, 0, 0, 0, NULL }

#if !defined(UNITTEST_DONT_USE_DEFAULT_MAIN)
extern unittest_test_t test_list[];
int main(int argc, char const *argv[]) {
  UnittestInit(argc, argv);
  return UnittestRun(test_list);
}
#endif  // UNITTEST_USE_DEFAULT_MAIN

static int failed_assertions;
static int failed_checks;
static int test_aborted;
static int total_assertions;
static int total_checks;
static int verbosity = 1;
static int quiet = 0;
jmp_buf jmp_env;
static unittest_test_t *current_test;

static char *AppendString(char *a, const char *b);

void ResetState() {
  test_aborted = 0;
  failed_assertions = 0;
  failed_checks = 0;
  current_test = NULL;
}

void SigHandler(int signum) {
  char buf[1024];
  int sz = 0;
  if (current_test) {
    sz += sprintf(buf + sz, "Error: ");
    switch (signum)
    {
    case SIGABRT:
      sz += sprintf(buf + sz, "SIGABRT raised");
      break;
    case SIGFPE:
      sz += sprintf(buf + sz, "SIGFPE raised");
      break;
    case SIGSEGV:
      sz += sprintf(buf + sz, "SIGSEGV raised");
      break;
    default:
      sz += sprintf(buf + sz, "General failure: signum [%d]", signum);
      break;
    }
    current_test->state = AppendString((char *)current_test->state, buf);
  }
  longjmp(jmp_env, -1);
}

static void UnittestInit(int argc, char const *argv[]) {
  // Register handlers 
  signal(SIGABRT, SigHandler); 
  signal(SIGFPE, SigHandler); 
  signal(SIGSEGV, SigHandler); 
}

static void ShowTestInfoShort(const unittest_test_t *test) {
  static const char padding[] =
      ".....................................................................";  
  int pad_length = sizeof(padding) - strlen(test->name);
  if (pad_length < 0) pad_length = 0;
  if (verbosity == 0) {
      fprintf(stderr, "%c", test->status);
  } else if (verbosity > 0) {
      fprintf(stderr, "%s %*.*s", test->name, pad_length, pad_length, padding);
      switch (test->status)
      {
      case 'F':
        fprintf(stderr, " FAILED\n");
        break;
      case 'E':
        fprintf(stderr, " ERROR\n");
        break;
      case '.':
        fprintf(stderr, " OK\n");
        break;
      
      default:
        fprintf(stderr, " ?????\n");
        break;
      }
  }
}

static int RunTest(unittest_test_t *test) {
  ResetState();
  current_test = test;
  if (setjmp(jmp_env) == 0) {
    test->test();
    if (failed_assertions > 0 || failed_checks > 0)
      test->status = 'F';
    else
      test->status = '.';
  } else {
    test->status = 'E';
  }
  test->failed_assertions = failed_assertions;
  test->failed_checks = failed_checks;
  
  if (!quiet) ShowTestInfoShort(test);
  return test->status != '.';
}

static int UnittestRun(unittest_test_t tests[]) {
  int i;
  int count = 0;
  int failed = 0;
  int first = 1;
  int _failed_assertions = 0;
  int _failed_checks = 0;
  unittest_test_t *test = tests;
  while (test->test != NULL) {
    failed += RunTest(test++);
    _failed_assertions += failed_assertions;
    _failed_checks += failed_checks;
    count++;
  }
  fprintf(stderr, "\n");
  test = tests;
  while (test->test != NULL) {
    if (test->status != '.') {
      if (first) {
        fprintf(stderr,
                "-----------------------------------------------------------------------\n");
        first = 0;
      }
      fprintf(stderr, "Test `%s` FAILED\n", test->name);
      fprintf(stderr, "%s\n", (const char *)test->state);
      fprintf(stderr,
              "-----------------------------------------------------------------------\n");
    }
    free(test->state);
    test++;
  }
  fprintf(stderr, "Executed %d tests, %d failed\n", count, failed);
  if (failed > 0) {
    fprintf(stderr, "Failed assertions: %d\n", _failed_assertions);
    fprintf(stderr, "Failed checks: %d\n", _failed_checks);
  }
  return failed;
}

static char *AppendString(char *a, const char *b) {
  size_t sz_a, sz_b;
  int offset = 0;
  sz_b = strlen(b);
  if (a == NULL) {
    sz_a = 0;
  } else {
    sz_a = strlen(a);
  }
  char *str = malloc(sz_a + sz_b + 2);

  if (sz_a != 0) offset += sprintf(str + offset, "%s\n", a);

  if (sz_b != 0) offset += sprintf(str + offset, "%s", b);

  if (a != NULL) free(a);

  return str;
}

static int UnittestEvaluate(const char *file, int line, int level,
                            const char *expr, int result, const char *msg) {
  static char buff[4096];
  int sz = 0;
  int offset = 0;
  const char *level_name;
  if (!result) {
    sz = sprintf(buff + sz, "[%s:%d] ", file, line);
    offset = sz;
    if (level == UNITTEST_LEVEL_ASSERTION) {
      failed_assertions++;
      sz += sprintf(buff + sz, "ASSERTION FAILED: ");
    } else if (level == UNITTEST_LEVEL_CHECK) {
      failed_checks++;
      sz += sprintf(buff + sz, "CHECK FAILED: ");
    }

    sz += sprintf(buff + sz, "`%s` is false.", expr);
    if (msg) sz += sprintf(buff + sz - 1, "\n%*.*s%s", offset, offset, "", msg);

    current_test->state = AppendString((char *)current_test->state, buff);
  }
  return failed_assertions > 0 || failed_checks > 0;
}

#endif  // UNITTEST_H
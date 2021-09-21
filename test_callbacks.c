#include "callbacks.h"
#include "unittest.h"

static int generator;

#define _CALLBACK_COUNT(_callback) _callback##__callback_count
#define _CALLBACK_STATE(_callback) _callback##__callback_state
#define _CALLBACK_RETVAL(_callback) _callback##__callback_retval
#define _CALLBACK_ORDER(_callback) _callback##__callback_order

#define DECLARE_CALLBACK(_callback)           \
  static int _CALLBACK_COUNT(_callback);      \
  static void* _CALLBACK_STATE(_callback);    \
  static int _CALLBACK_RETVAL(_callback);     \
  static int _CALLBACK_ORDER(_callback);      \
  int _callback(void* state) {                \
    _CALLBACK_COUNT(_callback) += 1;          \
    _CALLBACK_ORDER(_callback) = generator++; \
    _CALLBACK_STATE(_callback) = state;       \
    return _CALLBACK_RETVAL(_callback);       \
  }

DECLARE_CALLBACK(func1);
DECLARE_CALLBACK(func2);
DECLARE_CALLBACK(func3);
DECLARE_CALLBACK(func4);
DECLARE_CALLBACK(func5);

int evil_callback(void *state) {
    int *retval = (int*) state;
    *retval = RegisterCallback(func1, "evil-func1", NULL);
    return 1;
}

UNITTEST_TEST(CanRegisterCallback) {
  UNITTEST_ASSERT(CALLBACK_SUCCESS == RegisterCallback(func1, "func1", NULL));
  ReleaseCallbacks();
}

UNITTEST_TEST(CanRegisterSameCallbackMultipleTimes) {
  UNITTEST_ASSERT(CALLBACK_SUCCESS == RegisterCallback(func1, "func1", NULL));
  UNITTEST_ASSERT(CALLBACK_SUCCESS == RegisterCallback(func1, "func1", NULL));
  ReleaseCallbacks();
}

UNITTEST_TEST(CanRegisterMultipleCallbacks) {
  UNITTEST_ASSERT(CALLBACK_SUCCESS == RegisterCallback(func1, "func1", NULL));
  UNITTEST_ASSERT(CALLBACK_SUCCESS == RegisterCallback(func2, "func2", NULL));
  UNITTEST_ASSERT(CALLBACK_SUCCESS == RegisterCallback(func3, "func3", NULL));
  UNITTEST_ASSERT(CALLBACK_SUCCESS == RegisterCallback(func4, "func4", NULL));
  ReleaseCallbacks();
}

UNITTEST_TEST(CanExecuteCallback) {
  RegisterCallback(func1, "func1", NULL);
  _CALLBACK_RETVAL(func1) = 123465;
  _CALLBACK_COUNT(func1) = 0;
  UNITTEST_ASSERT_(ExecuteCallbacks(NULL) == 0,
                   "Callback should not be counter as error");
  UNITTEST_ASSERT_(_CALLBACK_COUNT(func1) == 1,
                   "Callback should have been executed once");
  ReleaseCallbacks();
}

UNITTEST_TEST(CanExecuteCallbackOnlyOnce) {
  RegisterCallback(func1, "func1", NULL);
  _CALLBACK_RETVAL(func1) = 123465;
  _CALLBACK_COUNT(func1) = 0;
  // Execute callbacks twice
  ExecuteCallbacks(NULL);
  ExecuteCallbacks(NULL);
  UNITTEST_ASSERT_(_CALLBACK_COUNT(func1) == 1,
                   "Callback should have been executed once");
  ReleaseCallbacks();
}

UNITTEST_TEST(CanExecuteCallbackLastInFirstOut) {
  RegisterCallback(func3, "func3", NULL);
  RegisterCallback(func2, "func2", NULL);
  RegisterCallback(func1, "func1", NULL);
  generator = 1;
  ExecuteCallbacks(NULL);
  UNITTEST_ASSERT_(_CALLBACK_ORDER(func1) == 1, "func1 should have been called first");
  UNITTEST_ASSERT_(_CALLBACK_ORDER(func2) == 2, "func2 should have been called second");
  UNITTEST_ASSERT_(_CALLBACK_ORDER(func3) == 3, "func3 should have been called third");
  ReleaseCallbacks();
}

UNITTEST_TEST(CanRegisterCallbackWithCustomId) {
  RegisterCallbackWithId(func1, "func1", NULL, 10);
  RegisterCallbackWithId(func2, "func2", NULL, 20);
  RegisterCallback(func3, "func3", NULL);
  
  generator = 1;
  _CALLBACK_COUNT(func3) = 0;

  ExecuteCallbacksWithId(NULL, 6);
  UNITTEST_ASSERT_(generator == 1, "No callbacks with ID 6 exist");
  
  ExecuteCallbacksWithId(NULL, 10);
  UNITTEST_ASSERT_(_CALLBACK_ORDER(func1) == 1, "func1 should have been called");
  
  ExecuteCallbacksWithId(NULL, 20);
  UNITTEST_ASSERT_(_CALLBACK_ORDER(func2) == 2, "func2 should have been called");
  
  UNITTEST_ASSERT_(_CALLBACK_COUNT(func3) == 0, "func3 should not have been called");
  ReleaseCallbacks();
}

UNITTEST_TEST(CallbackReturningZeroOrNegativeCountAsFailed) {
  RegisterCallback(func1, "func1", NULL);
  RegisterCallback(func2, "func2", NULL);
  RegisterCallback(func3, "func3", NULL);

  _CALLBACK_RETVAL(func1) = -1; // Error
  _CALLBACK_RETVAL(func2) = 1;  // Success
  _CALLBACK_RETVAL(func3) = 0;  // Error
  
  UNITTEST_ASSERT(ExecuteCallbacks(NULL) == 2);
  ReleaseCallbacks();
}

UNITTEST_TEST(CallbackReceiveCorrectState) {
  int a, b, c;
  RegisterCallback(func1, "func1", &a);
  RegisterCallback(func2, "func2", &b);
  RegisterCallback(func3, "func3", NULL);
  
  ExecuteCallbacks(&c);

  UNITTEST_ASSERT(_CALLBACK_STATE(func1) == &a);
  UNITTEST_ASSERT(_CALLBACK_STATE(func2) == &b);
  UNITTEST_ASSERT(_CALLBACK_STATE(func3) == &c);
  ReleaseCallbacks();
}

UNITTEST_TEST(CantRegisterCallbackWithinCallback) {
    int ret;
    RegisterCallback(evil_callback, "evil_callback", &ret);
    _CALLBACK_COUNT(func1) = 0;
    ExecuteCallbacks(NULL);
    UNITTEST_ASSERT(ret == CALLBACK_LOCKED);
    UNITTEST_ASSERT(_CALLBACK_COUNT(func1) == 0);
    ReleaseCallbacks();
}

UNITTEST_TEST(CanUnregisterCallback) {
  RegisterCallback(func1, "func1", NULL);
  RegisterCallback(func2, "func2", NULL);

  UNITTEST_ASSERT(CALLBACK_SUCCESS == UnregisterCallback(func1));
  
  ReleaseCallbacks();
}

UNITTEST_TEST(UnregisteredCallbacksArentExecuted) {
  RegisterCallback(func1, "func1", NULL);
  RegisterCallback(func2, "func2", NULL);

  _CALLBACK_COUNT(func1) = 0;
  _CALLBACK_COUNT(func2) = 0;

  UnregisterCallback(func1);
  ExecuteCallbacks(NULL);

  UNITTEST_ASSERT(_CALLBACK_COUNT(func1) == 0);
  UNITTEST_ASSERT(_CALLBACK_COUNT(func2) == 1);
  
  ReleaseCallbacks();
}

UNITTEST_TESTS = {
    UNITTEST_DECLARE(CanRegisterCallback),
    UNITTEST_DECLARE(CanRegisterSameCallbackMultipleTimes),
    UNITTEST_DECLARE(CanRegisterMultipleCallbacks),
    UNITTEST_DECLARE(CanExecuteCallback),
    UNITTEST_DECLARE(CanExecuteCallbackOnlyOnce),
    UNITTEST_DECLARE(CanExecuteCallbackLastInFirstOut),
    UNITTEST_DECLARE(CanRegisterCallbackWithCustomId),
    UNITTEST_DECLARE(CallbackReturningZeroOrNegativeCountAsFailed),
    UNITTEST_DECLARE(CallbackReceiveCorrectState),
    UNITTEST_DECLARE(CantRegisterCallbackWithinCallback),
    UNITTEST_DECLARE(CanUnregisterCallback),
    UNITTEST_DECLARE(UnregisteredCallbacksArentExecuted),
};
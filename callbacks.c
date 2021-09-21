#include "callbacks.h"

#define TCH_LOG(...)

#define STACK_ARRAY_INITIAL_SIZE 512
struct CALLBACK_NODE {
  int id;                     /* The id for this callback */
  int status;                 /* The status retuned by its execution */
  int executed;               /* Does it need to be executed? */
  int total_executions;       /* Total times it has been executed */
  char name[MAXSIZENAME];     /* Friendly name for callback */
  void *arg;                  /* Custom argument to be sent to the callback */
  CALLBACK_FUNC callback;     /* The callback function pointer */
  struct CALLBACK_NODE *next; /* Next element on the stack */
};

struct CALLBACK_STATE {
  struct CALLBACK_NODE *stack;
  struct CALLBACK_NODE *current_node;
  int policy;
  int status;
  int (*execPolicy)(void *, int);
};

enum {
  STACK_FREE,
  STACK_LOCKED,
};

#define FOREACH_NODE(node, stack) \
  for ((node) = (stack); (node); (node) = (node)->next)

#define SHOULD_EXECUTE(node, _id) \
  ((node)->executed == 0 &&       \
   (((_id) == 0 && (node)->id > 0) || (node)->id == (_id)))

static struct CALLBACK_STATE state;

// Locking functions
static int LockStack();
static int UnlockStack();
static int IsStackLocked();

// Getters and Setters
static struct CALLBACK_NODE *GetStack();
static void SetStack(struct CALLBACK_NODE *new_stack);
static struct CALLBACK_NODE *GetCurrent();
static void SetCurrent(struct CALLBACK_NODE *new_node);

// Policy Helpers
static void LoadExecPolicy();
static int ExecuteCallback(struct CALLBACK_NODE *node, void *parent_arg);
static int ExecuteCallbacksExecuteAll(void *arg, int id);
static int ExecuteCallbacksFailFast(void *arg, int id);

int RegisterCallback(CALLBACK_FUNC callback, const char *name, void *arg) {
  struct CALLBACK_NODE *node;

  if (!LockStack()) {
    /* If stack is busy, we can't change it. */
    return CALLBACK_LOCKED;
  }

  node = calloc(1, sizeof(struct CALLBACK_NODE));

  if (!node) {
    /* Calloc failed. Return failure */
    UnlockStack();
    return CALLBACK_FAILURE;
  }

  node->callback = callback;
  node->next = GetStack();
  node->arg = arg;
  strncpy(node->name, name, MAXSIZENAME);
  SetStack(node);

  UnlockStack();
  return CALLBACK_SUCCESS;
}

int RegisterCallbackWithId(CALLBACK_FUNC callback,
                           const char *name, void *arg, int id) {
  if (id == 0) {
    /* Cannot create callback with explicit id of zero. Return failure */
    return CALLBACK_FAILURE;
  }
  int status = RegisterCallback(callback, name, arg);
  if (status != CALLBACK_SUCCESS) {
    /* Error registring a new callback */
    return status;
  }

  /* Callback is on the top of the stack */
  struct CALLBACK_NODE *stack = GetStack();
  stack->id = id;
  return CALLBACK_SUCCESS;
}

int UnregisterCallback(CALLBACK_FUNC callback) {
  if (!LockStack()) {
    /* If stack is busy, we can't change it. */
    return CALLBACK_LOCKED;
  }

  struct CALLBACK_NODE *node;
  struct CALLBACK_NODE *prev = NULL;
  int status = CALLBACK_FAILURE;

  if (!node) {
    /* Stack empty. Return failure */
    UnlockStack();
    return CALLBACK_FAILURE;
  }

  FOREACH_NODE(node, GetStack()) {
    if (node->callback == callback) {
      if (prev != NULL) {
        prev->next = node->next;
      } else {
        SetStack(node->next);
      }
      free(node);
      node = NULL;
      status = CALLBACK_SUCCESS;
    }
    prev = node;
  }
  UnlockStack();
  return status;
}



int ExecuteCallbacksWithId(void *arg, int id) {
  if (!LockStack()) {
    /* If stack is busy, we can't change it. */
    return CALLBACK_LOCKED;
  }
  if (state.execPolicy == NULL) {
    LoadExecPolicy();
  }
  int errors = state.execPolicy(arg, id);

  UnlockStack();
  return errors;
}

int ExecuteCallbacks(void *arg) {
  /* To execute all callbacks with id >= 0, set the id to 0 */
  return ExecuteCallbacksWithId(arg, 0);
}

void ReleaseCallbacks() {
  if (!LockStack()) {
    /* If stack is busy, we can't change it. */
    return;
  }
  struct CALLBACK_NODE *node;
  struct CALLBACK_NODE *stack = GetStack();
  char message[1024];
  int offset;
  char executed;
  for (node = stack; node; node = stack) {
    stack = stack->next;
    offset =
        sprintf(message, "Releasing Callback[%s] ID[%d] Total Executions[%d]",
                node->name, node->id, node->total_executions);
    if (node->total_executions > 0)
      sprintf(message + offset, " Last ExitStatus[%d]", node->status);
    TCH_LOG(LOG_ALWAYS, "%s\n", message);
    free(node);
  }
  SetStack(NULL);
  UnlockStack();
}

int IsRunningAsCallback() { return IsStackLocked(); }

int ReRegisterItself() {
  struct CALLBACK_NODE *node = GetCurrent();
  if (!IsRunningAsCallback() || !node) {
    return CALLBACK_FAILURE;
  }
  node->executed = 0;
  return CALLBACK_SUCCESS;
}

int SetCallbackExecutionPolicy(int policy) {
  int old_policy = state.policy;
  state.policy = policy;

  LoadExecPolicy();

  return old_policy;
}

static int LockStack() {
  if (state.status != STACK_FREE) return 0;
  state.status = STACK_LOCKED;
  return 1;
}

static int UnlockStack() {
  state.status = STACK_FREE;
  return 1;
}

static int IsStackLocked() { return state.status == STACK_LOCKED; }

static struct CALLBACK_NODE *GetStack() { return state.stack; }

static void SetStack(struct CALLBACK_NODE *new_stack) {
  state.stack = new_stack;
}

static struct CALLBACK_NODE *GetCurrent() { return state.current_node; }

static void SetCurrent(struct CALLBACK_NODE *new_node) {
  state.current_node = new_node;
}

static void LoadExecPolicy() {
  switch (state.policy) {
    case CALLBACK_POLICY_FAIL_FAST:
      state.execPolicy = ExecuteCallbacksFailFast;
      break;
    case CALLBACK_POLICY_EXECUTE_ALL:
    default:
      state.execPolicy = ExecuteCallbacksExecuteAll;
      break;
  }
}

static int ExecuteCallback(struct CALLBACK_NODE *node, void *parent_arg) {
  if (node->executed == 0) {
    node->executed = 1;
    SetCurrent(node);
    node->status = node->callback(node->arg ?: parent_arg);
    node->total_executions++;
    SetCurrent(NULL);
    TCH_LOG(LOG_ALWAYS, "Callback[%s] ID[%d] ExitStatus[%d]\n", node->name,
            node->id, node->status);
  }
  return (node->status < 1);
}

static int ExecuteCallbacksFailFast(void *arg, int id) {
  TCH_LOG(LOG_ALWAYS, "CallbackExecutionPolicy: ExecuteCallbacksFailFast\n");
  struct CALLBACK_NODE *node;
  struct CALLBACK_NODE *stack = GetStack();
  int ret;
  FOREACH_NODE(node, stack) {
    if (SHOULD_EXECUTE(node, id)) {
      if ((ret = ExecuteCallback(node, arg)) < 1) return ret;
    }
  }
  return 1;
}

static int ExecuteCallbacksExecuteAll(void *arg, int id) {
  TCH_LOG(LOG_ALWAYS, "CallbackExecutionPolicy: ExecuteCallbacksExecuteAll\n");
  struct CALLBACK_NODE *node;
  struct CALLBACK_NODE *stack = GetStack();
  int errors = 0;
  FOREACH_NODE(node, stack) {
    if (SHOULD_EXECUTE(node, id)) {
      errors += ExecuteCallback(node, arg);
    }
  }
  return errors;
}

// END 
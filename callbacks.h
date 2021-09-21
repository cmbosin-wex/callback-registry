
#ifndef CALLBACK_REGISTRY_H
#define CALLBACK_REGISTRY_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef int (*CALLBACK_FUNC)(void*);

struct CALLBACK_REG;

// Maximum allowed size for a callback name
#define MAXSIZENAME 128


enum CALLBACK_EXEC_POLICY {
  CALLBACK_POLICY_EXECUTE_ALL,    /* Execute all callbacks and return the number 
                                     of failed callbacks. This is the default policy. */

  CALLBACK_POLICY_FAIL_FAST,      /* Execute callbacks and stop upon the first failure. 
                                     If nothing failed, return 1, otherwise return the 
                                     error code from the failing callback */
};

#define CALLBACK_FAILURE    0   // Something went wrong during callback setup 
#define CALLBACK_LOCKED    -1   // The underlying datastructure cannot be changed
#define CALLBACK_SUCCESS    1   // The operation succedded


int RegisterCallback(CALLBACK_FUNC callback, const char *name, void *arg);


int RegisterCallbackWithId(CALLBACK_FUNC callback, const char *name, void *arg, int id);


int UnregisterCallback(CALLBACK_FUNC callback);


int ExecuteCallbacks(void* arg);


int ExecuteCallbacksWithId(void* arg, int id);


void ReleaseCallbacks();


int IsRunningAsCallback();



int ReRegisterItself();


int SetCallbackExecutionPolicy(int policy);



#endif // CALLBACK_REGISTRY_H
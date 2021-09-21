#ifndef CALLBACK_REGISTRY_H
#define CALLBACK_REGISTRY_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Type of a callback. The callback must accept 
 * a ponter to void as argument and return an integer.
 * Only POSITIVE return values will be considered as success.
 * Anything else will be considered as failures.
 */
typedef int (*CALLBACK_FUNC)(void*);

/// Maximum allowed size for a callback name
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

/**
 * @brief Register a new callback function to be called. 
 * If the arg value is given, then that pointer is what will
 * be passed down to the callback. Otherwise, the pointer passed
 * to `ExecuteCallbacks` will be used. Be sure that arg will be 
 * a valid pointer when the callback is executed.
 * 
 * @param callback The callback function
 * @param arg Pointer to the arguments
 * @param name A nice name for the callback
 * @return Return CALLBACK_SUCCESS, CALLBACK_FAILURE or CALLBACK_LOCKED
 */
int RegisterCallback(CALLBACK_FUNC callback, const char *name, void *arg);

/**
 * @brief Register a new callback function to be called with a
 * specific ID. An ID may be POSITIVE or NEGATIVE, but never ZERO.
 * If this function is called with ID = 0, it will return a failure.
 * Moreover, if ID is negative, the only way to execute that callback 
 * is by explicitly calling ExecuteCallbacksById. ExecuteCallbacks only 
 * execute callbacks whose ID is either positive or the default one.
 * If the arg value is given, then that pointer is what will
 * be passed down to the callback. Otherwise, the pointer passed
 * to `ExecuteCallbacks` will be used. Be sure that arg will be 
 * a valid pointer when the callback is executed.
 * 
 * @param callback The callback function
 * @param arg Pointer to the arguments
 * @param name A nice name for the callback
 * @param id The nonzero value to be used as id for this callback.
 * @return Return CALLBACK_SUCCESS, CALLBACK_FAILURE or CALLBACK_LOCKED
 */
int RegisterCallbackWithId(CALLBACK_FUNC callback, const char *name, void *arg, int id);

/**
 * @brief Unregister a given callback from the queue.
 * If callback has been added multiple times, unregister
 * the most recent insertion.
 * 
 * @param callback The callback to be unregisted
 * @return Return CALLBACK_SUCCESS, CALLBACK_FAILURE or CALLBACK_LOCKED
 */
int UnregisterCallback(CALLBACK_FUNC callback);

/**
 * @brief Execute all nonexecuted registered callbacks whose ID is
 * either a positive value or the default ID.
 * 
 * @param arg Pointer to argument to be passed to all callbacks
 * @return Return the number of callbacks that didn't succeed.
 */
int ExecuteCallbacks(void* arg);

/**
 * @brief Execute all nonexecuted registered callbacks for the specified id 
 * 
 * @param arg Pointer to argument
 * @param id The id representing the callbacks
 * @return Return the number of callbacks that didn't succeed.
 */
int ExecuteCallbacksWithId(void* arg, int id);

/**
 * @brief Release all the resources and empty the stack of callbacks. 
 */
void ReleaseCallbacks();


/**
 * @brief Function to check if a given function was called natively
 * or as a callback
 * 
 * @return Return nonzero if is running as callback or zero otherwise.
 */
int IsRunningAsCallback();


/**
 * @brief This Function can be called by a callback to register itself
 * to be executed again. However, this will depend on ExecuteCallbacks 
 * being called a second time.
 * 
 * @return Return CALLBACK_SUCCESS or CALLBACK_FAILURE
 */
int ReRegisterItself();

/**
 * @brief Set the Callback Execution Policy object
 * 
 * @param policy Policy to be used
 * @return The old policy previously used.
 */
int SetCallbackExecutionPolicy(int policy);



#endif // CALLBACK_REGISTRY_H
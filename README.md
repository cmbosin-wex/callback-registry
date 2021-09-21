## Running

To build and execute the unit tests

```
make test
```

The expected output

```
CallbackSuite::CanRegisterCallback .................................... OK
CallbackSuite::CanRegisterSameCallbackMultipleTimes ................... OK
CallbackSuite::CanRegisterMultipleCallbacks ........................... OK
CallbackSuite::CanExecuteCallback ..................................... OK
CallbackSuite::CanExecuteCallbackOnlyOnce ............................. OK
CallbackSuite::CanExecuteCallbackLastInFirstOut ....................... OK
CallbackSuite::CanRegisterCallbackWithCustomId ........................ OK
CallbackSuite::CallbackReturningZeroOrNegativeCountAsFailed ........... OK
CallbackSuite::CallbackReceiveCorrectState ............................ OK
CallbackSuite::CantRegisterCallbackWithinCallback ..................... OK
CallbackSuite::CanUnregisterCallback .................................. OK
CallbackSuite::UnregisteredCallbacksArentExecuted ..................... OK
-----------------------------------------------------------------------
Executed 12 tests, 0 failed
```


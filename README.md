## Running

To build and execute the unit tests

```
make test
```

The expected output

```
CanRegisterCallback ................................................... OK
CanRegisterSameCallbackMultipleTimes .................................. OK
CanRegisterMultipleCallbacks .......................................... OK
CanExecuteCallback .................................................... OK
CanExecuteCallbackOnlyOnce ............................................ OK
CanExecuteCallbackLastInFirstOut ...................................... OK
CanRegisterCallbackWithCustomId ....................................... OK
CallbackReturningZeroOrNegativeCountAsFailed .......................... OK
CallbackReceiveCorrectState ........................................... OK
CantRegisterCallbackWithinCallback .................................... OK
-----------------------------------------------------------------------
Executed 10 tests, 0 failed
```


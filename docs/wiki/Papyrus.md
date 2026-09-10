# Papyrus

Dispatch a global function in a script supplied by your plugin:

```cpp
#include <CLibUtilsQTR/Papyrus.hpp>

// With your engine PCH in scope and the VM available:
const bool dispatched =
    Papyrus::CallFunction("MyPluginUtils", "NotifyReady");
```

The `MyPluginUtils.psc` source for this example contains:

```papyrus
Scriptname MyPluginUtils

Function NotifyReady() Global
    Debug.Trace("MyPlugin is ready")
EndFunction
```

Compile this source with the Papyrus compiler and include the resulting `Scripts/MyPluginUtils.pex` in your mod, so it is installed at `Data/Scripts/MyPluginUtils.pex`. The game loads the compiled `.pex`; the `.psc` source alone is not enough.

`CallFunction()` dispatches a static Papyrus function. Its boolean result describes dispatch, not script completion or the function's result. Additional arguments follow the function name and must match the script parameters.

Use `skyrim` and your engine PCH. This helper does not install scripts or register native functions.

## Bound script objects

A bound script object is the VM's instance of a script associated with an engine form.

| Operation | Use |
| --- | --- |
| `GetHandle(form)` | Obtain a VM handle |
| `GetObjectPtr(form, class_name, create)` | Find the bound object; optionally create and bind one |
| `GetAttachedScript(script_name, form)` | Inspect an attached script entry |

Supply a valid form and available VM. Set `create` false for inspection only; creating a binding changes state. Keep the smart pointer from `GetObjectPtr()` while using it. `GetAttachedScript()` exposes an internal raw pointer; do not retain it across VM state changes.

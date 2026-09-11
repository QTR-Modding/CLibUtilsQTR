# Dynamic form tracking

Create or retrieve a runtime copy of an existing form:

```cpp
#include <CLibUtilsQTR/DynamicFormTracker.hpp>

RE::FormID GetStaleFood(RE::AlchemyItem* base) {
    if (!base) return 0;

    constexpr std::uint32_t staleVariant = 1;
    auto* tracker = clib_utilsQTR::DynamicFormTracker::GetSingleton();
    return tracker->FetchCreate<RE::AlchemyItem>(
        base->GetFormID(), "", staleVariant);
}
```

Here `base` is an already-resolved food form. The result is a dynamic FormID, or `0` on failure. This creates or retrieves the form; it does not put an item in an inventory, rename it, or make it evolve. Apply your plugin's properties to the returned form before using it.

A **dynamic form** is a runtime-created record. The **base form** is the record it was copied from. DFT keeps a **bank** of these copies for each base so they can be reused.

Use the `skyrim` feature and include `CLibUtilsQTR/DynamicFormTracker.hpp`. No separate DFT feature or implementation file is needed. Follow [Getting started](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Getting-Started) and copy the current overlay port files; older pins do not contain this header.

Call engine-facing operations on the game thread when the relevant engine state is available. Internal locks protect tracker state; they do not make arbitrary engine operations safe on workers. The singleton belongs to your plugin, not a shared service coordinating every plugin.

## Choose a custom ID

A **custom ID** is a number your plugin assigns to distinguish copies of the same base. It is not a Skyrim FormID.

| Base | Custom ID | Your plugin's meaning |
| --- | --- | --- |
| Raw beef | `1` | Stale beef |
| Raw beef | `2` | Spoiled beef |
| Bread | `1` | Stale bread |

Requesting raw beef with ID `1` again returns that assignment when available. Bread can also use `1`: the assignment is scoped to its base. Keep the meaning stable across saves and plugin versions. Two features requesting the same base and custom ID share a form, so its properties affect both.

Use `std::nullopt` when you do not need a named assignment. Custom ID `0` is a valid assignment; it does not mean no ID. Repeated requests without an ID do not identify an already-active copy.

## Fetch versus create

| Call | Behavior |
| --- | --- |
| `Fetch(baseID, editorID, customID)` | Returns only the requested assignment, or `0`; never creates |
| `Fetch(baseID, editorID, std::nullopt)` | Finds an inactive copy with no custom ID, or returns `0` |
| `FetchCreate<T>(baseID, editorID, customID)` | Reuses that assignment, otherwise an unassigned inactive copy, otherwise creates |
| `FetchCreate<T>(baseID, editorID, std::nullopt)` | Reuses an unassigned inactive copy or creates one |

Successful fetching marks the form active. Use the engine type matching your base, such as `RE::AlchemyItem` or `RE::TESObjectARMO`. Supply a consistent base ID/editor ID pair; an empty editor ID lets the tracker obtain it from the base. When supplied, the editor ID takes lookup priority.

Fetching a reused form is not a promise to reset every property to the base. Apply all caller-owned properties needed by the new use. DFT copies engine form components, including armor biped and race data; it does not copy your plugin's separate state.

## Save and load

The plugin registers SKSE serialization callbacks and owns its record type and version. DFT supplies the record contents.

During your save callback:

```cpp
bool SaveForms(SKSE::SerializationInterface* serialization,
               std::uint32_t recordType, std::uint32_t recordVersion) {
    auto* tracker = clib_utilsQTR::DynamicFormTracker::GetSingleton();
    tracker->SendData();
    return tracker->Save(serialization, recordType, recordVersion);
}
```

`SendData()` prepares the serialized data. This `Save()` overload opens the record; the single-argument overload writes into an already-open record. Keep using your plugin's established record type/version when migrating compatible data.

Loading has two parts: reading the record, then restoring engine use of its forms.

1. At the start of a new load, stop your own pending work from using the previous save's state. Call `Reset()` once, and `Clear()` to discard previously buffered serialization data. `Reset()` retains the session's form bank; it does not delete all forms or clear that buffer.
2. In your SKSE record-reading loop, validate your record type and supported version, then call `Load(serialization)` for the DFT record. Handle a false return as a failed read. If the save has no DFT record, do not restore buffered data from a previous save.
3. After reading the records, call `ReceiveData()` for the loaded DFT data before your plugin recreates its assignments. This restores the saved associations in the tracker. A loaded save's association replaces a retained association for the same dynamic ID.
4. Restore your plugin's own saved uses. Use `Reserve(baseID, editorID, savedDynamicID)` for existing saved forms that must be kept while rebuilding. Fetch/create the required assignments and reapply names, effects, models, or other properties owned by your plugin.
5. If your plugin uses DFT's player active-effect restoration, read `GetPendingActiveEffects()` after `ReceiveData()`. Each record identifies the base (`baseFormid`), saved derivative (`dynamicFormid`), optional custom ID (`custom_id`), and elapsed seconds (`elapsed`). Recreate the required assignments and their properties even when no item remains in inventory. Then call `ApplyMissingActiveEffects()`. For newly written saves, each pending record also contains `effect`: the saved effect-list index, magic-effect FormID, and elapsed time. It restores only those recorded definitions, leaving existing matching effects alone. Finding or restoring an effect marks its derivative active in DFT, so later saves retain the effect. It is not a general NPC effect-restoration API.

A named form at a saved ID is accepted only if DFT already created or revived that exact live form in this session. Otherwise DFT leaves it untouched; use the ID returned by `FetchCreate()` for the replacement. Empty-named forms still follow the type-checked revival path. The ownership record survives `Reset()` and is removed when DFT deletes the form; it is not serialized.

Run the engine restoration steps at the appropriate load lifecycle point before normal inventory synchronization or other consumers can use the forms. Reading a record does not itself restore inventories or your plugin's item-to-form mappings. Persist those separately.

The serializer retains the existing AoT/Containerize form-bank layout and appends a versioned list of individual active effects. This list records each effect's derivative FormID, effect-list index, magic-effect FormID, and elapsed time. Dispelled effects and effects whose duration has elapsed are excluded. An effect whose index or magic-effect FormID no longer matches the restored item is skipped. Older records without this extension remain readable, but only support best-effort restoration from their item-level elapsed time; already-expired effect definitions are skipped. Arbitrary properties applied by your plugin are not serialized by this record. It uses the existing native binary representation, not a portable interchange format.

## Release, protect, and delete

**Active** means the tracker has yielded the form for use. **Protected** means it is reserved against ordinary inactive cleanup. Neither flag is an engine reference count: the tracker cannot decide when your plugin has finished using a form.

| Operation | Effect |
| --- | --- |
| `Reserve(baseID, editorID, dynamicID)` | Validates an existing form, copies base properties into it, associates it with that base, and protects it |
| `Unreserve(dynamicID)` | Removes protection |
| `SetInactive(dynamicID)` | Clears active, custom-ID, and protected state; retains the form in its base's bank |
| `DeleteInactives()` | Deletes inactive, unprotected tracked forms |
| `DeleteAll()` | Explicit teardown, including active and protected forms |

Reserve before reapplying custom properties, since reserving can copy base properties into the form. When fetching changes an inactive form to active, it removes its reservation. Protection is not a separate guarantee that a form can never be reused.

Call `SetInactive()` only after your plugin has removed or replaced all uses that must retain that assignment. It releases the custom ID as well, so a later request may reuse the form. Likewise, remove game-world, inventory, and other live uses before deleting forms. `DeleteAll()` is teardown, not a normal save-switch reset.

## Inspect the tracker

| Operation | Result |
| --- | --- |
| `GetOGFormOfDynamic(dynamicID)` | Original base form, or `nullptr` when untracked |
| `IsActive(dynamicID)` / `IsProtected(dynamicID)` | Tracker flags |
| `GetFormSet(baseID, editorID)` | Copy of that base's tracked IDs |
| `GetDynamicForms()` | Tracked dynamic IDs |
| `GetSourceForms()` | Base identities represented by tracked forms/effects |

Returned IDs can become invalid after deletion or save transitions. Resolve them when needed; do not retain dynamic-form pointers across those transitions.

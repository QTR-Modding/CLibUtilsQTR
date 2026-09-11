# Serialization

The existing string helpers read and write a paired binary representation inside an SKSE record:

```cpp
#include <SKSE/SKSE.h>
#include <CLibUtilsQTR/Serialization.hpp>

bool WriteLabel(SKSE::SerializationInterface* serialization) {
    // Caller already opened its record.
    return Serialization::write_string(serialization, "My label");
}

bool ReadLabel(SKSE::SerializationInterface* serialization,
               std::string& label) {
    // Caller selected the matching record for reading.
    return Serialization::read_string(serialization, label);
}
```

Use `skyrim`. The header includes the SKSE interface and uses `SKSE::log` directly; no caller-defined logger alias is needed. For runtime form creation and its save/load lifecycle, see [Dynamic form tracking](https://github.com/QTR-Modding/CLibUtilsQTR/wiki/Dynamic-Form-Tracking).

The plugin owns its callbacks, record type, version, and error handling. These helpers neither open records nor register callbacks. Check boolean returns before using results.

## Existing format

`write_string()` encodes at most 100 input characters, retaining supported printable characters. It writes a `std::size_t` count followed by raw `std::pair<int, bool>` entries. `read_string()` reads and decodes that representation.

This is an existing native binary format, not general UTF-8 or portable string storage. Pair the matching reader and writer when maintaining records using it. `encodeString()` and `decodeString()` expose the conversion separately.

## BaseData

`Serialization::BaseData<Key, Value>` supplies a protected map, recursive mutex, and public `SetData()` and `Clear()`. Derive from it to implement `GetType()` and the relevant `Save()`/`Load()` overloads. Base saving/loading implementations return false.

Use the protected `Locker` and `m_Lock` when accessing `m_Data` in derived methods. `GetData()` currently returns `float` and takes its missing-value argument as `Key`; it is not a generic value-returning lookup.

FormID resolution, engine object restoration, and deciding what to clear between saves belong to the consuming serializer.

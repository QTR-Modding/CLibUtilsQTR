# Form Groups (Quick Guide)

A form group = a named list of Forms. The group name is the filename (without `.txt`).

## Create a Group
1. Go to the configured groups folder (ask your dev; e.g. `Data/FormGroups`).
2. Make a text file: `WeaponsHeavy.txt` → group name = `WeaponsHeavy`.
3. Put one identifier per line (see formats below).
4. Save. Reload/restart the tool so it scans the folder.

## Accepted Line Formats (checked in this order)
1. **LocalID~Plugin**  
   Example: `012345~Skyrim.esm`  
   (Hex local ID + `~` + plugin filename)
2. **Full Hex FormID (7–8 hex digits)**  
   Example: `01ABCDEF`
3. **EditorID**  
   Example: `IronSword`

If a line matches an earlier format it stops there (so a value that looks like pure hex is treated as hex, not as an EditorID).

Blank lines ignored. Invalid lines silently skipped. Duplicates collapse automatically.

## Hex Notation Note
All hex numbers may optionally start with `0x` (e.g. `0x012345~Skyrim.esm` or `0x01ABCDEF`).  
We omit `0x` in examples for brevity and consistency, but both forms are accepted for:
- The LocalID part before `~`
- Raw / full FormIDs
Use only hexadecimal digits (0–9 A–F).

## Example File: `ArmorLight.txt`
```
HideArmor
LeatherArmor
012345~MyArmors.esp
01ABCD12
GlassArmor
```
(Above would also work with `0x012345~MyArmors.esp` and `0x01ABCD12`.)

## Using a Group
Where config allows a single Form or list, write the group name (e.g. `ArmorLight`) to expand to all its Forms.

## Using Groups in Other Config Files (YAML / JSON / etc.)
A mod author integrating this feature can allow you to reference group names anywhere their config expects one or more Forms. The runtime code first checks whether a token is a group name; if so it expands it to all Forms in that group. Otherwise it attempts to interpret it as a single Form (LocalID~Plugin, hex FormID, or EditorID).

### YAML Examples
Single value expecting one or many (expands if group):
```yaml
Loadout: ArmorLight
```
List mixing groups and singles:
```yaml
SpawnSets:
  - WeaponsHeavy      # group → many weapon Forms
  - ArmorLight        # group → many armor Forms
  - 012345~MyArmors.esp  # specific form from plugin
  - 01ABCDEF          # raw hex FormID
  - IronSword         # single EditorID
```
Grouped under keys:
```yaml
LootRules:
  Allowed:
    - WeaponsHeavy
    - ArmorLight
  Excluded:
    - 0x01ABCD12      # hex with 0x
    - SteelDagger     # EditorID
```

### JSON Examples
```json
{
  "spawn": {
    "includeGroups": ["ArmorLight", "WeaponsHeavy"],
    "includeMixed": ["ArmorLight", "012345~MyArmors.esp", "IronSword"],
    "exclude": ["01ABCDEF", "0x01ABCD12"]
  }
}
```
Another pattern (group as value):
```json
{
  "playerLoadout": "WeaponsHeavy"
}
```

Implementation detail: Because the loader checks for a group name before interpreting an identifier as a single Form, a group name that is also a valid EditorID or hex string will be treated as a group (collision – avoid giving a group a name that is exactly an existing EditorID you may still want to reference individually).

## Update
Edit the file → save → reload the tool.

## Troubleshooting
- Typo in an identifier → that line is ignored.
- Wrong folder or not `.txt` → file not scanned.
- Need a specific form from a specific plugin → use `LocalID~Plugin`.
- Group name collides with an EditorID/hex ID you also need individually → rename the group.

That’s it—just curated text files of identifiers, reusable across configs (YAML, JSON, etc).
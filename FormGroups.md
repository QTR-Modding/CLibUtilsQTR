# Form Groups (Quick Guide)

A form group = a named list of Forms. The group name is the filename (without `.txt`).

## Create a Group
1. Go to the configured groups folder (ask your dev; e.g. `Data/FormGroups`).
2. Make a text file: `WeaponsHeavy.txt` → group name = `WeaponsHeavy`.
3. Put one identifier per line (see formats below).


## Accepted Line Formats (checked in this order)
1. LocalID~Plugin  
   Example: `012345~Skyrim.esm`  
   (Hex local ID + `~` + plugin filename)
2. Full Hex FormID (7–8 hex digits)  
   Example: `01ABCDEF`
3. EditorID  
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

## Troubleshooting
- Typo in an identifier → that line is ignored.
- Wrong folder or not `.txt` → file not scanned.
- Need a specific form from a specific plugin → use `LocalID~Plugin`.

That’s it—just curated text files of identifiers.

## External Config Integration (JSON / YAML / etc.)
A mod author using this FormGroups feature may allow you to reference group names directly inside their other config files (YAML, JSON, TOML, INI-like) anywhere a Form (single) or list of Forms is expected. The typical resolution order is:
1. If a token matches a group name → expand to all Forms in that group.
2. Otherwise interpret it as a single identifier (LocalID~Plugin, full hex FormID, or EditorID).

### YAML Examples
```yaml
# Single value that may be a group
Loadout: ArmorLight

# Mixed list of groups and single identifiers
SpawnSets:
  - WeaponsHeavy        # group → expands to many weapon Forms
  - ArmorLight          # group
  - 012345~MyArmors.esp # explicit LocalID~Plugin
  - 01ABCDEF            # raw hex FormID
  - IronSword           # EditorID

LootRules:
  Allowed:
    - WeaponsHeavy
  Excluded:
    - 0x01ABCD12        # hex with optional 0x
    - SteelDagger       # EditorID
```

### JSON Examples
```json
{
  "spawn": {
    "include": ["ArmorLight", "WeaponsHeavy", "012345~MyArmors.esp", "IronSword"],
    "exclude": ["01ABCDEF", "0x01ABCD12"]
  },
  "playerLoadout": "WeaponsHeavy"
}
```

### Minimal INI / Key-Value Style
```
PlayerLoadout=ArmorLight
ExtraWeapons=WeaponsHeavy,012345~MyArmors.esp,IronSword
```

Collision tip: If a group name is identical to an EditorID or looks like a pure hex ID you still want to use individually, rename the group to avoid the name being consumed as a group first.

(Only this section was added; all previous sections remain unchanged.)
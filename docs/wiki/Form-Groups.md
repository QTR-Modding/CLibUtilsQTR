# Form Groups (User Quick Guide)

You can define reusable sets of Forms in plain `.txt` files, then reference the set by its filename inside supported YAML / JSON / INI / TOML configs. No code access needed.

## Make a Group
1. Go to the folder your mod/framework tells you (e.g. `Data/FormGroups`).
2. Create a text file: `ArmorLight.txt` → group name is `ArmorLight`.
3. Add one identifier per line (see formats). Save.

## Line Formats (checked in this order)
1. LocalID~Plugin  
   Example: `012345~Skyrim.esm`
2. Full Hex FormID (7–8 hex digits)  
   Example: `01ABCDEF`
3. EditorID  
   Example: `IronSword`

Optional `0x` prefix allowed on IDs: `0x01ABCDEF`, `0x012345~Skyrim.esm`.

Rules:
- First matching format wins (pure hex is NOT treated as an EditorID).
- Blank / invalid lines skipped silently.
- Duplicates removed automatically.

## Example Group File
`WeaponsHeavy.txt`
```
DaedricSword
012345~MyWeapons.esp
01ABCD77
SteelGreatsword
```

## Use in Configs
Where the consuming mod lets you specify a single Form or a list, you can write the group name instead. Mixed usage (groups + singles) is fine.

Resolution order when reading a token:
1. If it equals a group name → expand to all its lines.
2. Else if it matches LocalID~Plugin → single form.
3. Else if it matches full hex (with/without 0x) → single form.
4. Else → EditorID.

### YAML
```yaml
Loadout: ArmorLight

SpawnSets:
  - WeaponsHeavy    # group
  - ArmorLight      # group
  - 012345~MyArmors.esp
  - 01ABCDEF
  - IronSword
```

### JSON
```json
{
  "include": ["WeaponsHeavy", "012345~MyArmors.esp", "IronSword"],
  "exclude": ["01ABCDEF"]
}
```

### INI (example)
```
PlayerLoadout=ArmorLight
ExtraWeapons=WeaponsHeavy,012345~MyWeapons.esp,IronSword
```

### TOML
```toml
loadout = "ArmorLight"
spawn.include = ["WeaponsHeavy", "01ABCDEF", "IronSword"]
```

## Naming Tips
- Avoid names that are pure hex (`01ABCDEF`) or look like `012345~Plugin.esm`.
- Use clear descriptive names: `ArmorLight`, `Bosses`, `DraugrBosses`.
- If collision risk: prefix (e.g. `grp_ArmorLight`).

## Quick Troubleshooting
| Problem | Cause | Fix |
|---------|-------|-----|
| Group name not working | File not in correct folder / wrong extension | Move or rename to `.txt` |
| Missing entries | Typos / wrong plugin name | Correct lines |
| Token treated as group unexpectedly | Name collision with EditorID | Rename group |
| Nothing expands | Using unsupported feature (comments, includes) | Keep to basic lines only |

To verify, temporarily remove a line and see if the resulting behavior changes—helps confirm it’s being read.

## Minimal Checklist Before Sharing
- Descriptive filename.
- Only valid lines (no trailing spaces).
- No accidental duplicates (they’re harmless, but tidy helps).
- Provide a short README / note listing group names.

Done—just text files + names in configs. Keep it simple.

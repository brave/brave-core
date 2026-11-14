---
name: brave/json-resources-in-grd
description:
  'JSON data a feature ships is packaged as a resource in a .grd file, not
  shipped as a file and loaded from disk.'
rubric: design
severity: should_fix
---

**Rule.** Package a JSON data file the browser ships as an `<include>` in a
`.grd` file and read it through the resource bundle, as
`brave/components/brave_rewards/resources/brave_rewards_static_resources.grd`
does. Do not copy the JSON next to the binary and read it with
`base::ReadFileToString()` or a `JSONFileValueDeserializer`.

**Why.** A resource in a `.grd` is available to both C++ (through
`ui::ResourceBundle`) and JS (through WebUI), from one copy of the data, and
cannot be missing from disk at run time.

## Examples

Bad:

```cpp
base::FilePath path = assets_dir.AppendASCII("regions.json");
std::string json;
base::ReadFileToString(path, &json);
```

Good:

```xml
<include name="IDR_FOO_REGIONS_JSON" file="regions.json" type="BINDATA" />
```

```cpp
std::string json =
    ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
        IDR_FOO_REGIONS_JSON);
```

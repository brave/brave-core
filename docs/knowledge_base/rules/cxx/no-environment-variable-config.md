---
name: brave/no-environment-variable-config
description:
  'Configuration comes from GN args, with command line switches for runtime
  overrides, never from environment variables.'
rubric: design
severity: should_fix
---

**Rule.** Do not read configuration such as service URLs or keys from
environment variables (`getenv()`, `base::Environment`). Define it from a GN arg
at build time, and when it must be overridable at run time, read a command line
switch.

**Why.** Environment variables are invisible in the build configuration and
differ between the shells and launchers that start the browser. GN args and
switches are explicit and reproducible.

## Examples

Bad:

```cpp
std::string api_url = std::getenv("BRAVE_API_URL");
```

Good:

```cpp
// In BUILD.gn: defines += [ "BRAVE_API_URL=\"$brave_api_url\"" ]
std::string api_url = BRAVE_API_URL;
if (command_line.HasSwitch(switches::kBraveApiUrl)) {
  api_url = command_line.GetSwitchValueASCII(switches::kBraveApiUrl);
}
```

---
name: brave/enums-over-string-constants
description:
  'A value with a fixed set of valid options is an enum (with conversion to and
  from its string form where needed), not a raw string passed around the code.'
rubric: design
severity: should_fix
---

**Rule.** When a parameter, field or return value can only take one of a known
set of values (a wallet type, a mode, a state), declare an `enum class` for it
and convert to or from the string form only at the boundary that needs it
(prefs, JSON, IPC, UI). Do not pass the raw strings through the code and compare
them with `==`.

**Why.** An enum lets the compiler check that a `switch` handles every case and
rejects values outside the set. Raw strings accept typos and unknown values
silently, and every comparison has to repeat the literal.

## Examples

Bad:

```cpp
void SetWalletType(const std::string& type);
if (type == "uphold") { ... }
```

Good:

```cpp
enum class WalletType { kUphold, kGemini };
void SetWalletType(WalletType type);
```

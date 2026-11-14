---
name: brave/consistent-names-across-layers
description:
  'A concept keeps one name across C++, JS, prefs, Mojo and UI, instead of a
  different name in each layer.'
rubric: readability
severity: should_fix
---

**Rule.** When a change names a concept in more than one layer (a C++ method,
the pref storing it, the Mojo interface or WebUI message carrying it, the JS/TS
code and the UI string using it), every layer uses the same name, in that
language's casing. Do not call it one thing in C++ and another in JS or in the
pref.

**Why.** Different names for the same thing make readers wonder whether they are
different things, and make it hard to find every place a concept is handled when
it changes.

## Examples

Bad:

```cpp
// C++
bool IsTorDisabledManaged();
// JS
getTorManaged()
```

Good:

```cpp
// C++
bool IsTorManaged();
// JS
isTorManaged()
```

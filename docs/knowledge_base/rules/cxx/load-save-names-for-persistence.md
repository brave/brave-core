---
name: brave/load-save-names-for-persistence
description:
  'Methods that read and write persisted state come as a Load*/Save* pair.'
rubric: readability
severity: nit
---

**Rule.** Name the methods that restore and persist a piece of state as a
`Load*()`/`Save*()` pair with the same suffix: `LoadSavingsDaily()` and
`SaveSavingsDaily()`, not `ReadSavingsDaily()` with `StoreSavingsDaily()`, or
`RestoreX()` with `PersistX()`.

**Why.** Matching names make it obvious which methods are counterparts, and that
both must change when the persisted format does.

## Examples

Bad:

```cpp
void ReadSavingsDaily();
void PersistSavingsDaily();
```

Good:

```cpp
void LoadSavingsDaily();
void SaveSavingsDaily();
```

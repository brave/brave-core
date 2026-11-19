---
name: brave/scoped-uma-timer-for-slow-paths
description:
  'Code that processes data on the UI thread or may be slow is measured with
  SCOPED_UMA_HISTOGRAM_TIMER.'
rubric: perf
severity: nit
---

**Rule.** When a change adds a function that processes data on the UI thread, or
performs an operation that may be slow (parsing, matching, building large
structures), add a `SCOPED_UMA_HISTOGRAM_TIMER` at the top of its body, with a
`Brave.*` histogram name.

**Why.** Slow work on the UI thread makes the browser janky, and without a
measurement nobody notices how slow it is on real users' machines.

## Examples

Bad:

```cpp
void GetUrlCosmeticResourcesOnUI(const GURL& url) {
  // ... potentially slow work ...
}
```

Good:

```cpp
void GetUrlCosmeticResourcesOnUI(const GURL& url) {
  SCOPED_UMA_HISTOGRAM_TIMER(
      "Brave.CosmeticFilters.GetUrlCosmeticResourcesOnUI");
  // ... potentially slow work ...
}
```

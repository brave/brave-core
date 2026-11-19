---
name: brave/no-duplicated-upstream-code-in-overrides
description:
  A chromium_src override must never copy an entire upstream file, function or
  method; it keeps only the changed part and falls back to the upstream
  implementation for the rest.
rubric: design
severity: blocking
---

**Rule.** NEVER copy an entire upstream file, function or method into a
`chromium_src/` override to change part of it. The override holds only Brave's
logic for the part that changes, and calls the upstream implementation (its
`ChromiumImpl` fallback, or the superclass method) for everything else. This
does not license a new `_ChromiumImpl` rename to create that fallback, which is
banned in new code ([brave/no-chromium-impl-rename-to-wrap]).

**Why.** Copied code stops following upstream: each later rebase keeps running
the old, stale copy, upstream fixes to it are silently lost, and the rebase that
finally notices is much harder.

## Examples

Bad:

```cpp
// chromium_src/chrome/browser/ui/views/chrome_typography_provider.cc
SkColor ChromeTypographyProvider::GetColor(...) {
  // 50 lines copied from upstream, of which 3 differ.
}
```

Good:

```cpp
SkColor ChromeTypographyProvider::GetColor(...) {
  if (!ShouldIgnoreHarmonySpec(*native_theme)) {
    return ChromiumImpl::GetColor(...);
  }
  // Only Brave's logic here.
}
```

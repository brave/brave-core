---
name: brave/replace-disabled-classes-with-chromium-src-stubs
description:
  To disable or replace an upstream class entirely, provide minimal no-op .h and
  .cc replacements in chromium_src instead of patching the class.
rubric: design
severity: should_fix
---

**Rule.** When a change patches an upstream class's header and source to switch
the class off (or to swap its implementation wholesale), ask for `chromium_src/`
replacements of its `.h` and `.cc` instead: a minimal dummy class with the same
interface whose methods do nothing and return a neutral value. NEVER patch the
class's methods one by one to disable them.

**Why.** A dummy replacement needs no patch at all, so there is nothing to
conflict on a rebase, and it states in one place that Brave does not use the
class.

## Examples

Bad:

```
patches/components-translate-core-browser-translate_url_fetcher.cc.patch
patches/components-translate-core-browser-translate_url_fetcher.h.patch
```

Good:

```cpp
// chromium_src/components/translate/core/browser/translate_url_fetcher.h
class TranslateURLFetcher {
 public:
  TranslateURLFetcher() = default;
  bool Request(const GURL& url, Callback callback) { return false; }
};
```

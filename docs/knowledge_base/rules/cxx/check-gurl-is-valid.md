---
name: brave/check-gurl-is-valid
description:
  'A constructed GURL is checked with is_valid() before it is used, above all
  when built from input that may be malformed.'
rubric: safety
severity: should_fix
---

**Rule.** After constructing a `GURL`, check `is_valid()` and handle the invalid
case before using it, above all before passing it to loaders, navigation or
other network APIs, and whenever it is built from a string the code does not
control (user input, prefs, a server response, a page).

**Why.** A malformed URL makes an invalid `GURL`, and passing one to network
functions can crash or behave unexpectedly.

## Examples

Bad:

```cpp
GURL url(user_input);
loader->DownloadUrl(url);
```

Good:

```cpp
GURL url(user_input);
if (!url.is_valid()) {
  return;
}
loader->DownloadUrl(url);
```

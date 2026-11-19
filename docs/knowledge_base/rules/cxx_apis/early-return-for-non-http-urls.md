---
name: brave/early-return-for-non-http-urls
description:
  'URL processing in shields, debouncing and content settings code returns early
  for URLs that are not HTTP or HTTPS.'
rubric: perf
severity: should_fix
---

**Rule.** A function in shields, debouncing or content settings code that
inspects or rewrites a URL starts with `if (!url.SchemeIsHTTPOrHTTPS())`
returning its "nothing to do" result, before any parsing, lookup or rule
matching.

**Why.** These features only apply to web navigations and requests. Without the
early exit, `chrome://`, `brave://`, `file:`, `data:` or extension URLs reach
code that assumes a host and a registrable domain, wasting work and exposing
edge cases such as empty hosts or opaque origins.

## Examples

Bad:

```cpp
bool ShouldDebounce(const GURL& url) {
  return rules_.Matches(url.host());
}
```

Good:

```cpp
bool ShouldDebounce(const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }
  return rules_.Matches(url.host());
}
```

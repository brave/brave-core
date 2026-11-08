---
name: brave/reuse-existing-utilities
description:
  'New C++ helpers must not reimplement an operation that Chromium, //base or an
  existing Brave component already provides; call the existing utility instead.'
rubric: design
severity: should_fix
---

**Rule.** Before accepting a new helper that parses, formats, converts or
searches data (query strings, URLs, JSON, string splitting and joining, time and
number formatting, container operations), check whether `//base`, `//net`,
`//url` or another Chromium or Brave component already provides it. When one
does, the change calls the existing utility instead of adding custom code.

**Why.** The existing utilities are tested, handle the edge cases (escaping,
encodings, malformed input) that hand-written code usually misses, and are
maintained across Chromium upgrades. A custom copy adds code to review and keep
in sync, and often behaves subtly differently from the rest of the browser.

## Examples

Bad:

```cpp
std::string ParseQueryParam(const std::string& url, const std::string& key) {
  // custom parsing code...
}
```

Good:

```cpp
net::QueryIterator it(url);
while (!it.IsAtEnd()) {
  if (it.GetKey() == key) return it.GetValue();
  it.Advance();
}
```

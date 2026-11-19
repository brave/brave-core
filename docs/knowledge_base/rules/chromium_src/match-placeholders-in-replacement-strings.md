---
name: brave/match-placeholders-in-replacement-strings
description:
  'A Brave string substituted for an upstream string ID via #undef/#define must
  have placeholders compatible with what the upstream caller supplies.'
rubric: correctness
severity: blocking
---

**Rule.** When an override redirects an upstream string ID to a Brave one
(`#undef IDS_FOO` then `#define IDS_FOO IDS_BRAVE_FOO`), check the upstream code
that formats `IDS_FOO`: the Brave string MUST use only the placeholders (`$1`,
`$2`, ...) that code supplies, with the same meaning. Fewer is possible, as
extra values are ignored; more or different placeholders are not.

**Why.** The upstream caller keeps passing the values it passed for its own
string. Extra replacement values are silently dropped, but a placeholder the
caller does not supply, or one in a different position, can cause run-time
errors.

## Examples

```cpp
// chromium_src/chrome/browser/ui/foo/foo_dialog.cc
#undef IDS_FOO_DIALOG_TITLE
#define IDS_FOO_DIALOG_TITLE IDS_BRAVE_FOO_DIALOG_TITLE
```

Bad, when upstream formats `IDS_FOO_DIALOG_TITLE` with the origin alone:

```xml
<message name="IDS_BRAVE_FOO_DIALOG_TITLE" desc="Title of the Foo dialog">
  <ph name="ORIGIN">$1<ex>example.com</ex></ph> wants to use <ph name="DEVICE">$2<ex>your camera</ex></ph>
</message>
```

Good:

```xml
<message name="IDS_BRAVE_FOO_DIALOG_TITLE" desc="Title of the Foo dialog">
  <ph name="ORIGIN">$1<ex>example.com</ex></ph> wants to use a device
</message>
```

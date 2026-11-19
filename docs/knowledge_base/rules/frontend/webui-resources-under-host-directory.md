---
name: brave/webui-resources-under-host-directory
description:
  A WebUI's resource files must be placed in the top-level directory under
  resources/ that matches its WebUI host name, or they are not found at runtime.
rubric: correctness
severity: blocking
---

**Rule.** Place each new resource file of a WebUI in the top-level directory
under `resources/` that matches the WebUI's host name. Do NOT add it under the
directory of another WebUI, or under a top-level directory named after something
else.

**Why.** Misplaced resources are not found at runtime, so the page fails to load
them although the build succeeds.

## Examples

Bad:

```
# A resource of the brave://foo-panel WebUI, under another page's directory.
browser/resources/settings/foo_panel/foo_panel.tsx
```

Good:

```
browser/resources/foo_panel/foo_panel.tsx
```

---
name: brave/no-browser-only-apis-in-common
description:
  'Code in components/.../common/ must not use browser-process-only APIs
  (PrefService, content::BrowserContext, Profile, g_browser_process), nor gain
  DEPS or GN deps on them.'
rubric: design
severity: blocking
---

**Rule.** Code under `components/.../common/` MUST NOT use APIs that exist only
in the browser process: `PrefService`, `content::BrowserContext`, `Profile`,
`g_browser_process`, `content/public/browser/`, or the component's own
`browser/` code. Its dependencies must be safe for every process: `//base`,
mojom, shared structs. A new `+components/prefs` (or similar) line in a
`common/DEPS` file almost always means the helper belongs in `browser/`: move it
there.

**Why.** `common/` is compiled into every process that links the component
(browser, renderer, utility); browser-only types there are meaningless or unsafe
for the other processes.

## Examples

Bad:

```cpp
// components/playlist/core/common/utils.h
namespace playlist {
bool IsPlaylistEnabled(PrefService* prefs);  // PrefService is browser-only!
}
```

Good:

```cpp
// components/playlist/core/browser/utils.h
namespace playlist {
bool IsPlaylistEnabled(PrefService* prefs);
}
```

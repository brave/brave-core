---
name: brave/webui-preprocessor-for-feature-code
description:
  'Feature-specific WebUI code is compiled out with <if expr="enable_feature">
  blocks, rather than relying only on runtime loadTimeData checks.'
rubric: design
severity: should_fix
---

**Rule.** Wrap WebUI code that belongs to a buildflag-controlled feature in
Chromium's UI preprocessor: `// <if expr="enable_feature">` ... `// </if>` in JS
and TS, `<if expr="enable_feature">` ... `</if>` in HTML. Do not rely solely on
a runtime `loadTimeData` check to hide it.

**Why.** The preprocessor completely removes the code from the build when the
feature is disabled. A runtime check still ships the code, and its imports, in
builds where the feature does not exist.

## Examples

Bad:

```js
import { SpeedreaderPage } from './speedreader_page.js'

if (loadTimeData.getBoolean('isSpeedreaderEnabled')) {
  registerPage(SpeedreaderPage)
}
```

Good:

```js
// <if expr="enable_speedreader">
import { SpeedreaderPage } from './speedreader_page.js'
// </if>
```

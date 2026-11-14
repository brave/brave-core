---
name: brave/webui-icons-from-leo-icons
description:
  WebUI gets a new icon by adding its name to the Leo icon list in
  ui/webui/resources/BUILD.gn and rendering it with leo-icon, not by adding a
  new .svg file.
rubric: design
severity: should_fix
---

**Rule.** To use a new icon in WebUI, add the Leo icon's name to the Leo icon
list in `ui/webui/resources/BUILD.gn` (`leo_icons_sources`) and render it with
`<leo-icon name="...">`, or `Icon` from `@brave/leo/react/icon` in React. Do NOT
add an `.svg` icon file to a WebUI directory, or draw the icon inline. When
flagging a violation, tag `@brave/nala-token-reviewers`.

**Why.** Leo is the canonical icon source across all platforms. An icon added
outside it is inconsistent with the rest of the UI, duplicates icons Leo already
has, and is one more asset to maintain by hand.

## Examples

Bad:

```html
<!-- my_icon.svg added to the WebUI directory. -->
<img src="my_icon.svg" />
```

Good:

```html
<!-- my-icon-name added to the Leo icons of ui/webui/resources/BUILD.gn. -->
<leo-icon name="my-icon-name"></leo-icon>
```

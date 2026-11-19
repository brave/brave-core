---
name: brave/grd-webui-formatter-data
description:
  Strings added to a WebUI GRDP file carry formatter_data="webui=X", with X
  exactly the WebUI component's identifier.
rubric: correctness
severity: should_fix
---

**Rule.** When adding a `<message>` to a WebUI GRDP file, include
`formatter_data="webui=ComponentName"`, where `ComponentName` exactly matches
the WebUI component's identifier, as the other messages of the file use it
(`webui=AiChat`, not `webui=AIChat`).

**Why.** The build generates the string constants of each WebUI bundle from this
attribute; a missing or mistyped value leaves the string out of the component.

## Examples

Bad:

```xml
<message name="IDS_CHAT_UI_WELCOME" desc="Welcome message" formatter_data="webui=AIChat">
  Welcome to Brave AI
</message>
```

Good:

```xml
<message name="IDS_CHAT_UI_WELCOME" desc="Welcome message" formatter_data="webui=AiChat">
  Welcome to Brave AI
</message>
```

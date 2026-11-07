---
name: webui/flags-switching-shared-css
rubric: webui
severity: regression
---

**Rule.** Brave's overrides of shared WebUI CSS still win when a flag sets a
host attribute that enables `:host-context([…])` blocks.

**Why.** Those blocks' selectors outrank Brave's plain overrides, so the flag's
flip quietly replaces Brave's styles.

**How to check.** The candidates flip a WebUI feature, or change a
`:host-context([…])` block in a file Brave overrides. Intersect the classes in
such blocks with Brave's `chromium_src/ui/webui/resources/**/*.css`.

## Examples

- M158, 8127.2: `kWebUIRoundedIcons` replaced Brave's edit and folder icons.

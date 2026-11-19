---
name: brave/no-inline-scripts-in-webui-pages
description:
  JavaScript for a WebUI page comes from compiled TypeScript entry points added
  to its GN build, never from inline <script> tags.
rubric: design
severity: should_fix
---

**Rule.** Do not add an inline `<script>` with code to a WebUI page's HTML.
Write the code in TypeScript, add it as an entry point in the page's GN build
configuration (`entry_points`), and load the compiled bundle with
`<script src=...>`.

**Why.** Compiled entry points give the code type checking, code analysis and
the same bundling as the rest of the page; an inline script gets none of them.

## Examples

Bad:

```html
<script>
  document.body.classList.add('patched')
</script>
```

Good:

```gn
entry_points = [
  ["main", rebase_path("main.ts")],
  ["patches", rebase_path("patches.ts")],
]
```

```html
<script
  type="module"
  src="/patches.bundle.js"
></script>
```

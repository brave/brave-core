---
name: brave/keep-hardcoded-initial-colors-in-webui-html
description:
  WebUI HTML files keep hardcoded initial background and foreground colors for
  the page body or root container instead of Leo tokens, which load
  asynchronously.
rubric: correctness
severity: should_fix
---

**Rule.** In a WebUI page's HTML, keep the hardcoded initial `background` and
`color` of the page `body` or root container, set to the value the token
resolves to. Do NOT replace them with a design token such as
`var(--leo-color-container-background)`.

**Why.** Leo tokens are loaded asynchronously via JavaScript, so at initial
paint the token is not set yet, and the page shows a visible flash of unstyled
content (FOUC) until it loads. The hardcoded value is only visible for that
initial paint: once Leo tokens are initialized, the themed value takes over.

## Examples

Bad:

```html
<style>
  body {
    background: var(--leo-color-container-background);
  }
</style>
```

Good:

```html
<style>
  body {
    background: #111114;
  }
</style>
```

---
name: brave/ios-css-prefers-color-scheme-for-injected-content
description:
  'Dark mode in injected web content uses CSS @media (prefers-color-scheme:
  dark), not CSS classes toggled from JavaScript.'
rubric: design
severity: should_fix
---

**Rule.** Style the dark appearance of injected web content (user scripts,
interstitials, reader mode and similar pages) in CSS under
`@media (prefers-color-scheme: dark)`. Do not detect the theme in JavaScript or
Swift and add or remove a class such as `dark-mode`.

**Why.** The media query follows the system appearance as it changes, with no
script, no message from the app, and no flash of the wrong theme while the
script runs.

## Examples

Bad:

```js
if (window.matchMedia('(prefers-color-scheme: dark)').matches) {
  document.body.classList.add('dark-mode')
}
```

Good:

```css
@media (prefers-color-scheme: dark) {
  body {
    background-color: #1e2029;
  }
}
```

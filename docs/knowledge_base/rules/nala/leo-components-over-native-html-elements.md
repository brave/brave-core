---
name: brave/leo-components-over-native-html-elements
description:
  WebUI React code uses Leo's Button, Dropdown, Input, Checkbox, RadioButton,
  Toggle and Dialog instead of the native HTML elements they replace.
rubric: design
severity: should_fix
---

**Rule.** In WebUI React code, do NOT render a native element that a Leo
component replaces, directly or through `styled.*`. Use the Leo component
instead, and its own props and events (`isDisabled`, `e.detail.value`):

| Instead of                | Use                                               |
| ------------------------- | ------------------------------------------------- |
| `<select>`                | `Dropdown` from `@brave/leo/react/dropdown`       |
| `<button>`                | `Button` from `@brave/leo/react/button`           |
| `<input type="text">`     | `Input` from `@brave/leo/react/input`             |
| `<input type="checkbox">` | `Checkbox` from `@brave/leo/react/checkbox`       |
| `<input type="radio">`    | `RadioButton` from `@brave/leo/react/radioButton` |
| Toggle/switch UI          | `Toggle` from `@brave/leo/react/toggle`           |
| `<dialog>` / modal        | `Dialog` from `@brave/leo/react/dialog`           |

When flagging a violation, tag `@brave/nala-token-reviewers`.

**Why.** Leo components are design-system-approved, theme-aware and accessible.
Raw HTML produces visual inconsistency, bypasses Brave's design tokens, and
needs its accessibility done by hand.

## Examples

Bad:

```tsx
<button
  onClick={handleClick}
  disabled={isLoading}
>
  Submit
</button>
```

Good:

```tsx
import Button from '@brave/leo/react/button'
;<Button
  onClick={handleClick}
  isDisabled={isLoading}
>
  Submit
</Button>
```

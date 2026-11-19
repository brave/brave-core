---
name: brave/leo-components-over-custom-reimplementations
description:
  Before building a custom React component, check Leo; use its ButtonMenu,
  Tooltip, SegmentedControl, ProgressRing, Menu, Dialog, Alert or Collapse
  instead of a hand-rolled equivalent.
rubric: design
severity: should_fix
---

**Rule.** Do NOT build a custom React component for something Leo already
provides. Use the Leo component instead of building:

- a button with a dropdown menu: `ButtonMenu` from
  `@brave/leo/react/buttonMenu`;
- a tooltip or hover popover: `Tooltip` from `@brave/leo/react/tooltip`;
- a segmented control or tab switcher: `SegmentedControl` and
  `SegmentedControlItem` from `@brave/leo/react/segmentedControl` and
  `@brave/leo/react/segmentedControlItem`;
- a loading spinner or activity indicator: `ProgressRing` from
  `@brave/leo/react/progressRing`;
- a dropdown or context menu: `Menu` from `@brave/leo/react/menu`;
- a modal or overlay dialog: `Dialog` from `@brave/leo/react/dialog`;
- a toast or notification banner: `Alert` from `@brave/leo/react/alert`;
- a collapsible or accordion: `Collapse` from `@brave/leo/react/collapse`.

When flagging a violation, tag `@brave/nala-token-reviewers`.

**Why.** Custom re-implementations add maintenance burden, produce visual
inconsistency, bypass Brave's design tokens, and are often less accessible than
the Leo originals.

## Examples

Bad:

```tsx
function MyTooltip({ text, children }) {
  const [visible, setVisible] = React.useState(false)
  return (
    <div
      onMouseEnter={() => setVisible(true)}
      onMouseLeave={() => setVisible(false)}
    >
      {children}
      {visible && <div className='tooltip'>{text}</div>}
    </div>
  )
}
```

Good:

```tsx
import Tooltip from '@brave/leo/react/tooltip'
;<Tooltip text='Helpful hint'>
  <Button>Hover me</Button>
</Tooltip>
```

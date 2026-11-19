---
name: brave/storybook-stories-for-new-ui
description:
  New UI features should have Storybook stories covering their primary visual
  states; this is a preference, and a reviewer never insists on more stories.
rubric: testing
severity: optional
---

**Rule.** When a change adds a new UI feature without Storybook stories
(`*.stories.tsx`) for its primary visual states, suggest adding them. A single
integrated story that exercises several closely-coupled components together
(such as a panel story covering the full panel with all its sub-components) is
enough; separate per-component stories are not always necessary. This is a
preference, not a requirement: NEVER insist on additional stories if the
developer considers the existing coverage adequate.

**Why.** Stories serve both as documentation and as visual regression baselines
for the UI they render.

## Examples

Good:

```tsx
// components/foo/stories/foo_panel.stories.tsx
export default {
  title: 'Foo/Panel',
  component: FooPanel,
}

export const Default = {}
export const Empty = { args: { items: [] } }
export const Loading = { args: { isLoading: true } }
```

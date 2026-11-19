---
name: brave/reorder-data-for-ui-on-client-side
description:
  'Sorting, grouping or prioritizing data for display happens in the UI layer;
  the core data layer and API responses keep the canonical order.'
rubric: design
severity: should_fix
---

**Rule.** Do not sort, group or reorder data in a service, the core data layer
or an API response to suit how one UI displays it. Return the data in its
canonical order, and reorder it in the UI code that presents it.

**Why.** Presentation order is a UI concern: baked into the backend, it is
imposed on every other client, and changing the UI then means changing the core
layer.

## Examples

Bad:

```cpp
// components/foo/core/browser/foo_service.cc
std::ranges::sort(items, {}, &mojom::Item::pinned);  // pinned first, for the UI
std::move(callback).Run(std::move(items));
```

Good:

```ts
// components/foo/resources/page/items_list.tsx
const sorted = [...items].sort((a, b) => Number(b.pinned) - Number(a.pinned))
```

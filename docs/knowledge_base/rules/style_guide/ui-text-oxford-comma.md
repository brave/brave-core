---
name: brave/ui-text-oxford-comma
description:
  A list of three or more items in user-facing text has a comma before its last
  item (the Oxford comma).
rubric: readability
severity: nit
---

**Rule.** In a list of three or more items, always put a comma before the "and"
or "or" that introduces the last item.

**Why.** Without the serial comma, the last two items can read as one, or the
first item as a description of the rest.

## Examples

Bad:

```xml
<message name="IDS_SYNC_DATA_LIST" desc="List of the data types sync covers">
  Sync your bookmarks, passwords and history.
</message>
```

Good:

```xml
<message name="IDS_SYNC_DATA_LIST" desc="List of the data types sync covers">
  Sync your bookmarks, passwords, and history.
</message>
```

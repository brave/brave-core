---
name: brave/bump-resource-ids-by-five
description:
  'A new resource ID in resource_ids.spec bumps the next ID by 5, not by 1.'
rubric: correctness
severity: nit
---

**Rule.** When a change adds resource IDs to `resource_ids.spec`, bump the next
ID by 5, never by 1, so a gap is left after the new entry.

**Why.** The gap leaves room for later additions without the IDs of one entry
running into those of the adjacent entry.

## Examples

Bad:

```python
"brave/components/foo/resources/foo_resources.grd": {
  "includes": [53000],
},
"brave/components/bar/resources/bar_resources.grd": {
  "includes": [53001],
},
```

Good:

```python
"brave/components/foo/resources/foo_resources.grd": {
  "includes": [53000],
},
"brave/components/bar/resources/bar_resources.grd": {
  "includes": [53005],
},
```

---
name: brave/check-value-changed-before-notifying
description:
  'Before notifying observers of a state change, compare the old and new values,
  and notify only when the value changed.'
rubric: perf
severity: should_fix
---

**Rule.** When a setter or update path notifies observers (or calls an
`On*Changed()` method) after updating a value, keep the old value, update, and
notify only if the new value differs.

**Why.** Notifying when nothing changed sends observers needless work, such as
UI re-renders, and can cascade into further redundant updates.

## Examples

Bad:

```cpp
visual_content_used_percentage_ = new_value;
OnStateForConversationEntriesChanged();
```

Good:

```cpp
auto old_value = visual_content_used_percentage_;
visual_content_used_percentage_ = new_value;
if (old_value != visual_content_used_percentage_) {
  OnStateForConversationEntriesChanged();
}
```

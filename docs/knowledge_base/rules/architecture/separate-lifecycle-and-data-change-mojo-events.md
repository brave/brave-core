---
name: brave/separate-lifecycle-and-data-change-mojo-events
description:
  'A Mojo Changed event fires only when data actually changes; lifecycle events,
  such as the model loading, get an event of their own.'
rubric: design
severity: should_fix
---

**Rule.** In a Mojo listener or observer interface, a `Changed` (or
`On*Changed`) event is sent only when the data changed. Do not also send it for
lifecycle events such as the model finishing loading or a listener registering;
declare a separate event, such as `OnReady()`, for them.

**Why.** A client cannot tell a lifecycle event sent as `Changed` from a real
change, so it reloads or re-renders needlessly, or misreads initial state as a
user's edit.

## Examples

Bad:

```mojom
interface BookmarksListener {
  Changed(BookmarksChange change);  // Fires on model load AND data change
};
```

Good:

```mojom
interface BookmarksListener {
  OnBookmarksReady();
  OnBookmarksChanged(BookmarksChange change);
};
```

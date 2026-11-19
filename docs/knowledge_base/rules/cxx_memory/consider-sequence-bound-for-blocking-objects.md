---
name: brave/consider-sequence-bound-for-blocking-objects
description:
  'A class doing blocking or IO work that the UI thread calls asynchronously can
  be held in a base::SequenceBound<T> instead of hand-posted tasks.'
rubric: design
severity: optional
---

**Rule.** When a class performs blocking or IO operations and the UI thread
calls it asynchronously, consider holding it in a `base::SequenceBound<T>` bound
to a background task runner and calling it with `AsyncCall()`, instead of having
callers post each call to the thread pool and bind a `WeakPtr` by hand.

**Why.** `SequenceBound` binds the object to one task runner and posts every
call to that sequence automatically, which removes the manual thread management
in which lifetime and threading mistakes are made.

## Examples

Avoid:

```cpp
class ContentScraper {
  void Process(const std::string& html);  // blocking
};
// Caller must manually post to thread pool and bind weak ptr
```

Prefer:

```cpp
base::SequenceBound<ContentScraper> scraper_;
scraper_.AsyncCall(&ContentScraper::Process).WithArgs(html);
```

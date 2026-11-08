---
name: brave/ios-comment-artificial-delays
description:
  'Every artificial delay, such as Task.sleep or a delayed dispatch, has a
  comment explaining why it is necessary.'
rubric: readability
severity: should_fix
---

**Rule.** A `Task.sleep`, `DispatchQueue.asyncAfter`,
`perform(_:with:afterDelay:)`, `dispatch_after` or `PostDelayedTask` added to
wait for something MUST have a comment saying what it waits for and why no
callback or event can be used instead.

**Why.** An unexplained delay is a code smell that often hides a race condition;
the comment lets reviewers judge it and later readers know when it can go.

## Examples

Bad:

```swift
try await Task.sleep(nanoseconds: 500_000_000)
```

Good:

```swift
// Wait for WebKit to finish layout after orientation change.
// WebKit does not provide a completion callback for this operation.
try await Task.sleep(nanoseconds: 500_000_000)
```

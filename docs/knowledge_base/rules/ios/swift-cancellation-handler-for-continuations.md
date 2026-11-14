---
name: brave/swift-cancellation-handler-for-continuations
description:
  'Swift code that wraps a cancellable callback API in a checked continuation
  propagates Task cancellation with withTaskCancellationHandler.'
rubric: correctness
severity: should_fix
---

**Rule.** When a `withCheckedContinuation` or `withCheckedThrowingContinuation`
wraps a callback-based operation, wrap it in `withTaskCancellationHandler` whose
`onCancel:` cancels the underlying operation (`request.cancel()`).

**Why.** A continuation does not see the cancellation of the `Task` awaiting it:
without the handler, cancelling the task leaves the request running and the task
suspended until the request finishes on its own.

## Examples

Bad:

```swift
func fetchData() async throws -> Data {
  try await withCheckedThrowingContinuation { continuation in
    request.start { result in
      continuation.resume(with: result)
    }
  }
}
```

Good:

```swift
func fetchData() async throws -> Data {
  try await withTaskCancellationHandler {
    try await withCheckedThrowingContinuation { continuation in
      request.start { result in
        continuation.resume(with: result)
      }
    }
  } onCancel: {
    request.cancel()
  }
}
```

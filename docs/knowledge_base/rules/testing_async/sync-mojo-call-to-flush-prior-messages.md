---
name: brave/sync-mojo-call-to-flush-prior-messages
description:
  To be sure async Mojo messages have been processed, a test makes a synchronous
  call on the same interface, relying on Mojo's in-order delivery, instead of
  sleeping or polling.
rubric: testing
severity: should_fix
---

**Rule.** When a test must know that asynchronous messages it sent over a Mojo
interface have been processed (typically across processes), make a synchronous
call through that same interface and assert on its result, instead of sleeping
or polling with a delay. Mojo processes the messages of one interface in order,
so the synchronous call returns only after every earlier message on it. The
guarantee holds only for the same interface: a call on a different interface or
pipe does not order anything.

**Why.** Sleeping only guesses how long the messages take and is flaky; polling
is slower and less precise than a call that is ordered after the messages by
construction.

## Examples

Bad:

```cpp
// Init() was sent asynchronously on the Solana provider interface.
base::PlatformThread::Sleep(base::Milliseconds(100));
```

Good:

```cpp
// isConnected is a synchronous call on the same provider interface, so it
// runs after Init().
EXPECT_EQ(true,
          content::EvalJs(web_contents, "window.braveSolana.isConnected"));
```

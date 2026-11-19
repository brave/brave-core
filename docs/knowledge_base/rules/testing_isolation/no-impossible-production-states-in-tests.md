---
name: brave/no-impossible-production-states-in-tests
description:
  Tests reach the state they need through the code paths production uses, never
  by forcing internal state into a configuration production cannot reach.
rubric: testing
severity: should_fix
---

**Rule.** A test sets up its scenario by driving the same calls production makes
(loading, navigating, receiving a response). It MUST NOT poke internal members
or test hooks into a combination of values production code can never produce,
such as a "completed" flag without the result it implies, and then test how the
code reacts to it.

**Why.** Behaviour in an impossible state says nothing about the real program:
such tests prove nothing, give false confidence, and break or mislead when the
internals change.

## Examples

Bad:

```cpp
// Production never marks a download complete without a file path.
download->state_ = DownloadState::kComplete;
download->file_path_ = base::FilePath();
EXPECT_FALSE(manager_->CanOpen(*download));
```

Good:

```cpp
download->OnResponseStarted(MakeResponse());
download->OnComplete(base::FilePath(FILE_PATH_LITERAL("file.txt")));
EXPECT_TRUE(manager_->CanOpen(*download));
```

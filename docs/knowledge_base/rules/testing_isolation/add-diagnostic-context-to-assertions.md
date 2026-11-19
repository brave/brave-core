---
name: brave/add-diagnostic-context-to-assertions
description:
  Test assertions stream the state that explains a failure (inputs, codes,
  iteration) with <<, so failures are debuggable from the log.
rubric: readability
severity: nit
---

**Rule.** Where the compared values alone do not identify what failed, an
assertion streams the relevant context with `<<`: the input it was checking (the
URL, the case name), and the state that explains the result (the response code,
the error). Flag an assertion whose failure output would not tell the reader
which input or state produced it.

**Why.** gtest prints only the two compared values. Without the inputs and state
around them, a failure on a bot cannot be diagnosed from the log, and has to be
reproduced and re-run under a debugger.

## Examples

Bad:

```cpp
EXPECT_EQ(result.status, Status::kSuccess);
```

Good:

```cpp
EXPECT_EQ(result.status, Status::kSuccess)
    << "Failed for URL: " << url << ", response code: " << result.code;
```

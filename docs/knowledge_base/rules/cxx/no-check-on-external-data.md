---
name: brave/no-check-on-external-data
description:
  "CHECK is used only for conditions within the code's control; data from
  databases, user input or other external sources is handled gracefully."
rubric: correctness
severity: blocking
---

**Rule.** NEVER `CHECK()` (or `NOTREACHED()` on) a condition that depends on
data from outside the code's control: values read from a database, prefs or
files, user input, network responses, or messages from another process. Test the
condition and handle the failure: log it if useful, and return an error,
`std::nullopt` or a default.

**Why.** External data can be missing or malformed for reasons no code change
can rule out. A `CHECK()` on it turns bad input into a browser crash.

## Examples

Bad:

```cpp
// db_value is read from the database.
CHECK(db_value.has_value());
```

Good:

```cpp
if (!db_value.has_value()) {
  DLOG(ERROR) << "Missing expected database value";
  return std::nullopt;
}
```

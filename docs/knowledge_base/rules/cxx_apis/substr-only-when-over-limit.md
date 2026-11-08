---
name: brave/substr-only-when-over-limit
description:
  'When truncating a string to a limit, call std::string::substr() only if the
  content exceeds the limit and keep the original otherwise; when the result is
  only read, slice a std::string_view instead.'
rubric: perf
severity: should_fix
---

**Rule.** When a `std::string` is truncated to a maximum length:

- If the truncated result is only read, take a `std::string_view` of the content
  and call `substr()` on the view.
- If an owned string is needed, call `std::string::substr()` only when
  `content.size()` exceeds the limit, and otherwise use (move) the original.

Do not call `std::string::substr(0, max_length)` unconditionally. Note that
binding `cond ? content.substr(...) : content` to a `const std::string&` still
copies `content`, because the conditional expression is a new string.

**Why.** `std::string::substr()` always allocates and copies, even when the
content is already within the limit, which is the common case for limits on page
text or prompts. A view slices without allocating.

## Examples

Bad:

```cpp
std::string truncated = content.substr(0, max_length);
```

Good:

```cpp
std::string truncated = content.size() > max_length
                            ? content.substr(0, max_length)
                            : std::move(content);
std::string_view preview = std::string_view(content).substr(0, max_length);
```

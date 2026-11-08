---
name: brave/no-silent-fallback-on-unknown-types
description:
  'Code mapping inputs to types reports unknown or unsupported inputs
  explicitly, for example with std::nullopt, instead of silently falling back to
  a default type.'
rubric: correctness
severity: should_fix
---

**Rule.** When code classifies an input (a MIME type, a kind, an enum read from
outside) and meets one it does not know or support, return an explicit error,
such as `std::nullopt`, and let the caller handle it. Do not map unknown inputs
to a default type, as a final `return` or a `default:` case, as if they were
known.

**Why.** A silent fallback treats bad input as valid: the bug surfaces later,
far from its cause, and is hard to debug.

## Examples

Bad:

```cpp
FileType GetFileType(const std::string& mime) {
  if (mime == "application/pdf") return FileType::kPDF;
  return FileType::kImage;  // Silent fallback!
}
```

Good:

```cpp
std::optional<FileType> GetFileType(const std::string& mime) {
  if (mime == "application/pdf") return FileType::kPDF;
  if (mime.starts_with("image/")) return FileType::kImage;
  return std::nullopt;  // Caller handles unknown types
}
```

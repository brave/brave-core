---
name: brave/convert-utf-strings-at-boundaries
description:
  'Convert between UTF-8 and UTF-16 with base::UTF8ToUTF16()/base::UTF16ToUTF8()
  at the boundary that needs the other encoding, rather than storing both forms
  of the same text.'
rubric: design
severity: nit
---

**Rule.** Keep a piece of text in one encoding, and convert it with
`base::UTF8ToUTF16()` or `base::UTF16ToUTF8()` where it crosses into code that
needs the other one (UI labels, network or storage). Do not add a second member
or field that holds the same text in the other encoding.

**Why.** Two copies of the same text have to be kept in sync on every update,
double the memory, and leave readers unsure which one is authoritative. A
conversion at the boundary is cheap and keeps one source of truth.

## Examples

Bad:

```cpp
struct Bookmark {
  std::string title;
  std::u16string title16;  // same text, UTF-16
};
```

Good:

```cpp
struct Bookmark {
  std::u16string title;
};
// At the boundary that needs UTF-8:
std::string title_utf8 = base::UTF16ToUTF8(bookmark.title);
```

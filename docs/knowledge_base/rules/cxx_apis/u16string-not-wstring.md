---
name: brave/u16string-not-wstring
description:
  'UTF-16 text is std::u16string/char16_t* with u"..." literals; std::wstring
  and wchar_t* are only for Windows-only code.'
rubric: correctness
severity: should_fix
---

**Rule.** Hold 16-bit text in `std::u16string`, `std::u16string_view` and
`char16_t*`, and write its literals as `u"..."` (not
`base::ASCIIToUTF16("...")`). Use `std::wstring`, `wchar_t*` and `L"..."` only
in Windows-only code (`_win` files, `#if BUILDFLAG(IS_WIN)` blocks) where a
Windows API requires them.

**Why.** UTF-16 is what Chromium uses for UI text. `wchar_t` is 16 bits on
Windows but 32 bits elsewhere, so `std::wstring` in cross-platform code means
different encodings on different platforms; even on Windows it is a distinct
type from `std::u16string`, and needs conversions.

## Examples

Bad:

```cpp
std::wstring title = L"Brave";
```

Good:

```cpp
std::u16string title = u"Brave";
```

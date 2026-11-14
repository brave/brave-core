---
name: brave/no-byte-escapes-in-utf16-literals
description:
  'Write non-ASCII characters in u"..." literals as the character itself or a
  \uXXXX / \UXXXXXXXX escape, never as \xXX byte escapes.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER use `\x` escapes for non-ASCII characters in a UTF-16 literal
(`u"\xe2\x80\xa2"`). Write the character itself (`u"•"`) or its code point as
`\u2022` (or `\U0001F600` beyond the Basic Multilingual Plane).

**Why.** In a `u"..."` literal, each `\xXX` is a whole UTF-16 code unit, not a
byte: the UTF-8 bytes of a character written this way become several wrong
characters (`u"\xe2\x80\xa2"` is "â", a control character and "¢", not "•"), and
the compiler accepts it silently.

## Examples

Bad:

```cpp
std::u16string bullet = u"\xe2\x80\xa2";
```

Good:

```cpp
std::u16string bullet = u"•";
std::u16string bullet_escaped = u"\u2022";
```

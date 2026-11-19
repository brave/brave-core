---
name: brave/standard-scheme-separator-constant
description:
  'Build URLs with url::kStandardSchemeSeparator (and the url::k*Scheme
  constants), not a hard-coded "://" literal.'
rubric: readability
severity: nit
---

**Rule.** When C++ code assembles a URL string from parts, join the scheme and
the host with `url::kStandardSchemeSeparator` (with schemes such as
`url::kHttpsScheme`) instead of the literal `"://"`.

**Why.** The named constants are what Chromium's URL code uses, are searchable,
and avoid typos in hand-written separators and schemes.

## Examples

Bad:

```cpp
std::string url = scheme + "://" + host + path;
```

Good:

```cpp
std::string url = base::StrCat({url::kHttpsScheme,
                                url::kStandardSchemeSeparator, host, path});
```

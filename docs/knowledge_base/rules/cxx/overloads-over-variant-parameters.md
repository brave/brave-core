---
name: brave/overloads-over-variant-parameters
description:
  'Distinct call patterns get overloads rather than one function taking a
  std::variant parameter.'
rubric: design
severity: nit
---

**Rule.** When callers pass one of a few distinct kinds of argument, declare an
overload for each kind instead of a single function taking a `std::variant` of
them.

**Why.** Overloads state each supported call pattern in the signature and let
the compiler pick the implementation; a variant parameter moves that dispatch
into the function body.

## Examples

Bad:

```cpp
void Navigate(std::variant<GURL, content::OpenURLParams> target);
```

Good:

```cpp
void Navigate(const GURL& url);
void Navigate(const content::OpenURLParams& params);
```

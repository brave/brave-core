---
name: brave/name-params-in-header-declarations
description:
  'Function declarations in headers name all of their parameters, with the names
  used in the .cc definition.'
rubric: readability
severity: nit
---

**Rule.** Every parameter of a function declared in a header has a name, and the
name matches the one in the definition. Unnamed parameters are a problem above
all when several share a type (two `std::optional<std::string>`, two `bool`) or
when the type does not say what the value is. Deleted copy operations and
`MOCK_METHOD` declarations are not affected.

**Why.** The header is the API's documentation; parameter names are what tell a
caller which argument goes where. Names that differ from the definition make the
declaration mislead.

## Examples

Bad:

```cpp
void OnSubmitSignedExtrinsic(std::optional<std::string>,
                             std::optional<std::string>);
```

Good:

```cpp
void OnSubmitSignedExtrinsic(std::optional<std::string> transaction_hash,
                             std::optional<std::string> error_str);
```

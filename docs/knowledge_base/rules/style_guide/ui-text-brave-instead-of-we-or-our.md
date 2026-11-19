---
name: brave/ui-text-brave-instead-of-we-or-our
description:
  User-facing strings say "Brave's" instead of "our", and in legal, security or
  data privacy text say "Brave" or "the Brave team" instead of "we".
rubric: readability
severity: should_fix
---

**Rule.** Refer to Brave by name rather than in the first person plural:

- Write "Brave's", not "our": "access Brave's premium products", not "access our
  premium products".
- In legal, security and data privacy text, write "Brave" or "the Brave team",
  NEVER "we" or "us".

**Why.** "We" in privacy text implies that Brave staff can access the user's
data. Naming Brave is also clearer about who provides the product.

## Examples

Bad:

```xml
<message name="IDS_SEARCH_DATA_NOTICE" desc="Notice in the search privacy settings">
  We can only access the following search data…
</message>
```

Good:

```xml
<message name="IDS_SEARCH_DATA_NOTICE" desc="Notice in the search privacy settings">
  Brave can only access the following search data…
</message>
```

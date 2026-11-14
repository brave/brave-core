---
name: brave/no-substring-matching-on-json
description:
  Tests never validate JSON with testing::HasSubstr or other substring checks;
  they compare parsed values.
rubric: testing
severity: blocking
---

**Rule.** NEVER check JSON in a test by searching its text, with
`testing::HasSubstr` or otherwise. Parse it and compare the value, against
`base::test::ParseJsonDict()` of the expected JSON.

**Why.** A substring check depends on how the serializer spaces and orders the
output, so it breaks on harmless changes, and it does not verify the structure:
it passes when the key is nested in the wrong object, duplicated, or followed by
unexpected fields.

## Examples

Bad:

```cpp
EXPECT_THAT(json_str, testing::HasSubstr("\"role\":\"user\""));
```

Good:

```cpp
EXPECT_EQ(base::test::ParseJsonDict(json_str),
          base::test::ParseJsonDict(R"({"role":"user","content":"hi"})"));
```

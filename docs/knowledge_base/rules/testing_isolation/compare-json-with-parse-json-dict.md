---
name: brave/compare-json-with-parse-json-dict
description:
  Tests write the expected JSON value as a string parsed with
  base::test::ParseJsonDict(), not as a dictionary built key by key.
rubric: readability
severity: should_fix
---

**Rule.** When a test compares a JSON value, write the expected value as JSON
text and parse it with `base::test::ParseJsonDict()` in the assertion. Flag an
expected `base::Value::Dict` assembled with a run of `Set()` calls only to be
compared.

**Why.** The JSON literal reads like the payload it describes and shows its
whole structure at a glance; a dictionary built key by key is longer, hides the
nesting, and is easy to get subtly wrong.

## Examples

Bad:

```cpp
base::Value::Dict expected;
expected.Set("method", "chain_getBlockHash");
expected.Set("id", 1);
EXPECT_EQ(actual, expected);
```

Good:

```cpp
EXPECT_EQ(actual, base::test::ParseJsonDict(
                      R"({"method":"chain_getBlockHash","id":1})"));
```

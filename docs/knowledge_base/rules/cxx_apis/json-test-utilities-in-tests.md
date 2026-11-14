---
name: brave/json-test-utilities-in-tests
description:
  'Tests parse JSON with base::test::ParseJson()/ParseJsonDict() and check
  dictionaries with the matchers of base/test/values_test_util.h, not with
  JSONReader and manual Find*() assertions.'
rubric: testing
severity: should_fix
---

**Rule.** In C++ tests, build expected or input values with
`base::test::ParseJson()`, `base::test::ParseJsonDict()` or
`base::test::ParseJsonList()`, and assert on dictionaries with the
`base/test/values_test_util.h` matchers (`base::test::DictionaryHasValue()`,
`base::test::DictionaryHasValues()`, `base::test::IsJson()`). Do not parse with
`base::JSONReader` and then walk the result with `ASSERT_TRUE(value->is_dict())`
and `FindString()` checks.

**Why.** The helpers fail the test with a clear message on malformed JSON, and
the matchers print the whole value and the mismatch, where a chain of manual
`ASSERT_TRUE()` checks only reports which line failed.

## Examples

Bad:

```cpp
auto value = base::JSONReader::Read(json_str);
ASSERT_TRUE(value);
ASSERT_TRUE(value->is_dict());
auto* name = value->GetDict().FindString("name");
ASSERT_TRUE(name);
EXPECT_EQ(*name, "test");
```

Good:

```cpp
auto dict = base::test::ParseJsonDict(json_str);
EXPECT_THAT(dict, base::test::DictionaryHasValue("name", base::Value("test")));
```

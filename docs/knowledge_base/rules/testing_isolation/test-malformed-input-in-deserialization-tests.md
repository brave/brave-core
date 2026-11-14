---
name: brave/test-malformed-input-in-deserialization-tests
description:
  Deserialization code is tested with invalid and malformed input as well as
  valid input.
rubric: testing
severity: should_fix
---

**Rule.** When a change adds or tests deserialization (from JSON, a
`base::Value`, a proto, a mojom), its tests include invalid and malformed input
(a bad URL, a wrong type, a missing field) and assert that it is rejected or
handled. Flag deserialization tests that only feed well-formed input.

**Why.** Deserializers read stored or received data that can be corrupt, old or
hostile. The malformed-input path is where they crash or accept garbage, and
only a test that feeds it bad input covers it.

## Examples

Bad:

```cpp
auto result = Deserialize(R"({"url": "https://example.com"})");
ASSERT_NE(result, std::nullopt);
EXPECT_EQ(result->url, "https://example.com");
```

Good:

```cpp
auto result = Deserialize(R"({"url": "https://example.com"})");
ASSERT_NE(result, std::nullopt);
EXPECT_EQ(result->url, "https://example.com");

auto bad_result = Deserialize(R"({"url": "not-a-valid-url"})");
EXPECT_EQ(bad_result, std::nullopt);
```

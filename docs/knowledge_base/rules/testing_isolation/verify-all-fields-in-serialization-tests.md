---
name: brave/verify-all-fields-in-serialization-tests
description:
  Serialization and deserialization tests assert every field of the result,
  including secondary ones such as favicon URLs.
rubric: testing
severity: should_fix
---

**Rule.** A test of serialization, deserialization or a round trip asserts every
field of the type, not only the primary ones: secondary fields such as
`favicon_url` or `description` need assertions too. Flag such a test that leaves
fields of the type unchecked.

**Why.** Secondary fields are the ones a serializer forgets or maps to the wrong
key, and a test that skips them lets that loss reach users' saved data.

## Examples

Bad:

```cpp
EXPECT_EQ(result.title, "Test");
EXPECT_EQ(result.url, "https://example.com");
```

Good:

```cpp
EXPECT_EQ(result.title, "Test");
EXPECT_EQ(result.url, "https://example.com");
EXPECT_EQ(result.favicon_url, "https://example.com/favicon.ico");
```

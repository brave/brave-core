---
name: brave/extract-repeated-assertions-into-helpers
description:
  Blocks of assertions a test file repeats for each item are extracted into a
  reusable verification helper.
rubric: readability
severity: nit
---

**Rule.** When tests repeat the same group of assertions for each item they
check (a type check followed by field checks, for example), move the group into
a helper such as `VerifyTextBlock(block, text)` and call it per item. Flag a
change that adds the same assertion block several times over.

**Why.** Repeated blocks bury what differs between items in boilerplate, and a
fix to the checks has to be made in every copy. A named helper states what is
being verified and keeps each call to one line.

## Examples

Bad:

```cpp
EXPECT_TRUE(blocks[0]->is_text_content_block());
EXPECT_EQ(blocks[0]->get_text_content_block()->text, "hello");
EXPECT_TRUE(blocks[1]->is_image_content_block());
EXPECT_EQ(blocks[1]->get_image_content_block()->url, "img.png");
```

Good:

```cpp
void VerifyTextBlock(const ContentBlock& block, const std::string& text) {
  ASSERT_TRUE(block.is_text_content_block());
  EXPECT_EQ(block.get_text_content_block()->text, text);
}

VerifyTextBlock(*blocks[0], "hello");
VerifyImageBlock(*blocks[1], "img.png");
```

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/conversation_api_parsing.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

using Content = ConversationAPIClient::Content;

// Helper function to create a text content block
mojom::ContentBlockPtr CreateTextContentBlock(const std::string& text) {
  return mojom::ContentBlock::NewTextContentBlock(
      mojom::TextContentBlock::New(text));
}

// Helper function to create an image content block
mojom::ContentBlockPtr CreateImageContentBlock(const std::string& url) {
  return mojom::ContentBlock::NewImageContentBlock(
      mojom::ImageContentBlock::New(GURL(url)));
}

}  // namespace

TEST(ConversationApiParsingTest, ContentBlocksToJson_EmptyStringVector) {
  std::vector<std::string> empty_strings;
  Content content = empty_strings;

  base::Value result = ContentBlocksToJson(content);

  EXPECT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), "");
}

TEST(ConversationApiParsingTest, ContentBlocksToJson_SingleString) {
  std::vector<std::string> single_string = {"Hello, world!"};
  Content content = single_string;

  base::Value result = ContentBlocksToJson(content);

  EXPECT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), "Hello, world!");
}

TEST(ConversationApiParsingTest, ContentBlocksToJson_MultipleStrings) {
  std::vector<std::string> multiple_strings = {"First string", "Second string",
                                               "Third string"};
  Content content = multiple_strings;

  base::Value result = ContentBlocksToJson(content);

  EXPECT_THAT(result,
              base::test::IsJson(
                  R"(["First string", "Second string", "Third string"])"));
}

TEST(ConversationApiParsingTest, ContentBlocksToJson_EmptyContentBlocks) {
  std::vector<mojom::ContentBlockPtr> empty_blocks;
  Content content = std::move(empty_blocks);

  base::Value result = ContentBlocksToJson(content);

  EXPECT_THAT(result, base::test::IsJson("[]"));
}

TEST(ConversationApiParsingTest, ContentBlocksToJson_SingleTextBlock) {
  std::vector<mojom::ContentBlockPtr> blocks;
  blocks.push_back(CreateTextContentBlock("Hello from text block"));
  Content content = std::move(blocks);

  base::Value result = ContentBlocksToJson(content);

  EXPECT_THAT(result,
              base::test::IsJson(
                  R"([{"type": "text", "text": "Hello from text block"}])"));
}

TEST(ConversationApiParsingTest, ContentBlocksToJson_SingleImageBlock) {
  std::vector<mojom::ContentBlockPtr> blocks;
  blocks.push_back(CreateImageContentBlock("https://example.com/image.jpg"));
  Content content = std::move(blocks);

  base::Value result = ContentBlocksToJson(content);

  EXPECT_THAT(
      result,
      base::test::IsJson(
          R"([{"type": "image_url", "image_url": {"url": "https://example.com/image.jpg"}}])"));
}

TEST(ConversationApiParsingTest, ContentBlocksToJson_MixedContentBlocks) {
  std::vector<mojom::ContentBlockPtr> blocks;
  blocks.push_back(CreateTextContentBlock("First text"));
  blocks.push_back(CreateImageContentBlock("https://example.com/image1.jpg"));
  blocks.push_back(CreateTextContentBlock("Second text"));
  blocks.push_back(CreateImageContentBlock(
      "data:image/jpeg;base64,/9j/4AAQSkZJRgABAgAAZABkAAD"));
  Content content = std::move(blocks);

  base::Value result = ContentBlocksToJson(content);

  EXPECT_THAT(result, base::test::IsJson(R"([
      {"type": "text", "text": "First text"},
      {"type": "image_url", "image_url": {
        "url": "https://example.com/image1.jpg"
      }},
      {"type": "text", "text": "Second text"},
      {"type": "image_url", "image_url": {
        "url": "data:image/jpeg;base64,/9j/4AAQSkZJRgABAgAAZABkAAD"
      }}
    ])"));
}

TEST(ConversationApiParsingTest, ContentBlocksToJson_InvalidImageURL) {
  std::vector<mojom::ContentBlockPtr> blocks;
  blocks.push_back(CreateImageContentBlock("invalid-url"));
  Content content = std::move(blocks);

  base::Value result = ContentBlocksToJson(content);

  EXPECT_THAT(result,
              base::test::IsJson(
                  R"([{"type": "image_url", "image_url": {"url": ""}}])"));
}

TEST(ConversationApiParsingTest, ContentBlocksToJson_EmptyTextBlock) {
  std::vector<mojom::ContentBlockPtr> blocks;
  blocks.push_back(CreateTextContentBlock(""));
  Content content = std::move(blocks);

  base::Value result = ContentBlocksToJson(content);

  EXPECT_THAT(result, base::test::IsJson(R"([{"type": "text", "text": ""}])"));
}

}  // namespace ai_chat

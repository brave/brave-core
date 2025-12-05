/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/browser/engine/extended_content_block.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kTestText[] = "This is test text for rewriting.";

struct RewriteActionTestParam {
  mojom::ActionType action_type;
  std::optional<ExtendedContentBlockType> expected_content_type;
  std::string expected_payload;  // non-empty for change tones
};

}  // namespace

class OAIMessageUtilsTest : public testing::Test {
 public:
  OAIMessageUtilsTest() = default;
  ~OAIMessageUtilsTest() override = default;
};

class BuildOAIRewriteSuggestionMessagesTest
    : public OAIMessageUtilsTest,
      public testing::WithParamInterface<RewriteActionTestParam> {};

TEST_P(BuildOAIRewriteSuggestionMessagesTest,
       BuildsCorrectMessageForActionType) {
  RewriteActionTestParam param = GetParam();

  auto messages =
      BuildOAIRewriteSuggestionMessages(kTestText, param.action_type);
  // Verify invalid action types would return nullopt.
  if (!param.expected_content_type) {
    EXPECT_FALSE(messages);
    return;
  }

  ASSERT_TRUE(messages);
  // Verify we get exactly one message
  ASSERT_EQ(messages->size(), 1u);

  const auto& message = messages->at(0);
  EXPECT_EQ(message.role, "user");

  // Verify message has two content blocks
  ASSERT_EQ(message.content.size(), 2u);

  // First block should be page excerpt with the text
  EXPECT_EQ(message.content[0].type, ExtendedContentBlockType::kPageExcerpt);
  auto* excerpt_content = std::get_if<TextContent>(&message.content[0].data);
  ASSERT_TRUE(excerpt_content);
  EXPECT_EQ(excerpt_content->text, kTestText);

  // Second block should be the action-specific content
  EXPECT_EQ(message.content[1].type, param.expected_content_type);

  if (param.expected_content_type == ExtendedContentBlockType::kChangeTone) {
    auto* tone_content =
        std::get_if<ChangeToneContent>(&message.content[1].data);
    ASSERT_TRUE(tone_content);
    EXPECT_EQ(tone_content->tone, param.expected_payload);
  } else {
    auto* action_content = std::get_if<TextContent>(&message.content[1].data);
    ASSERT_TRUE(action_content);
    EXPECT_EQ(action_content->text, param.expected_payload);
  }
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BuildOAIRewriteSuggestionMessagesTest,
    testing::Values(
        RewriteActionTestParam{mojom::ActionType::PARAPHRASE,
                               ExtendedContentBlockType::kParaphrase, ""},
        RewriteActionTestParam{mojom::ActionType::IMPROVE,
                               ExtendedContentBlockType::kImprove, ""},
        RewriteActionTestParam{mojom::ActionType::ACADEMICIZE,
                               ExtendedContentBlockType::kChangeTone,
                               "academic"},
        RewriteActionTestParam{mojom::ActionType::PROFESSIONALIZE,
                               ExtendedContentBlockType::kChangeTone,
                               "professional"},
        RewriteActionTestParam{mojom::ActionType::PERSUASIVE_TONE,
                               ExtendedContentBlockType::kChangeTone,
                               "persuasive"},
        RewriteActionTestParam{mojom::ActionType::CASUALIZE,
                               ExtendedContentBlockType::kChangeTone, "casual"},
        RewriteActionTestParam{mojom::ActionType::FUNNY_TONE,
                               ExtendedContentBlockType::kChangeTone, "funny"},
        RewriteActionTestParam{mojom::ActionType::SHORTEN,
                               ExtendedContentBlockType::kShorten, ""},
        RewriteActionTestParam{mojom::ActionType::EXPAND,
                               ExtendedContentBlockType::kExpand, ""},
        RewriteActionTestParam{mojom::ActionType::CREATE_TAGLINE, std::nullopt,
                               ""}));

}  // namespace ai_chat

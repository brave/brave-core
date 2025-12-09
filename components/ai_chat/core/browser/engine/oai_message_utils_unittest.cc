/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/engine/extended_content_block.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kTestText[] = "This is test text for rewriting.";
constexpr char kSeedText[] = "Here is the rewritten version:";

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

TEST_F(OAIMessageUtilsTest, BuildOAISeedMessage) {
  OAIMessage message = BuildOAISeedMessage(kSeedText);

  EXPECT_EQ(message.role, "assistant");
  ASSERT_EQ(message.content.size(), 1u);
  EXPECT_EQ(message.content[0].type, ExtendedContentBlockType::kText);

  auto* text_content = std::get_if<TextContent>(&message.content[0].data);
  ASSERT_TRUE(text_content);
  EXPECT_EQ(text_content->text, kSeedText);
}

TEST_F(OAIMessageUtilsTest, BuildOAIMessages) {
  // Create page contents for different turns
  PageContent page_content1("Page content 1", false);
  PageContent video_content1("Video transcript 1", true);
  PageContent page_content3("Page content 3", false);
  PageContent page_content4("Page content 4", false);

  // Build page contents map (turn2 is assistant, no page contents)
  PageContentsMap page_contents_map;
  page_contents_map["turn1"] = {std::cref(page_content1),
                                std::cref(video_content1)};
  page_contents_map["turn3"] = {std::cref(page_content3)};
  page_contents_map["turn4"] = {std::cref(page_content4)};

  // Build conversation history with 4 turns
  EngineConsumer::ConversationHistory history;

  // Turn 1: Human with page + video + selected_text + regular action
  auto turn1 = mojom::ConversationTurn::New();
  turn1->uuid = "turn1";
  turn1->character_type = mojom::CharacterType::HUMAN;
  turn1->action_type = mojom::ActionType::QUERY;
  turn1->text = "What is this?";
  turn1->selected_text = "Selected excerpt";
  history.push_back(std::move(turn1));

  // Turn 2: Assistant with no page contents
  auto turn2 = mojom::ConversationTurn::New();
  turn2->uuid = "turn2";
  turn2->character_type = mojom::CharacterType::ASSISTANT;
  turn2->text = "This is the answer.";
  history.push_back(std::move(turn2));

  // Turn 3: Human with page content + SUMMARIZE_PAGE action
  auto turn3 = mojom::ConversationTurn::New();
  turn3->uuid = "turn3";
  turn3->character_type = mojom::CharacterType::HUMAN;
  turn3->action_type = mojom::ActionType::SUMMARIZE_PAGE;
  turn3->text = "Summarize";
  history.push_back(std::move(turn3));

  // Turn 4: Human with page content + no selected_text
  auto turn4 = mojom::ConversationTurn::New();
  turn4->uuid = "turn4";
  turn4->character_type = mojom::CharacterType::HUMAN;
  turn4->action_type = mojom::ActionType::QUERY;
  turn4->text = "Another question";
  history.push_back(std::move(turn4));

  bool sanitize_input_called = false;
  std::vector<OAIMessage> messages = BuildOAIMessages(
      std::move(page_contents_map), history, 10000,
      [&sanitize_input_called](std::string&) { sanitize_input_called = true; });

  EXPECT_TRUE(sanitize_input_called);

  // Should have 4 messages
  ASSERT_EQ(messages.size(), 4u);

  // Message 1: Human turn with all content types
  EXPECT_EQ(messages[0].role, "user");
  ASSERT_EQ(messages[0].content.size(), 4u);
  EXPECT_EQ(messages[0].content[0].type,
            ExtendedContentBlockType::kVideoTranscript);
  EXPECT_EQ(messages[0].content[1].type, ExtendedContentBlockType::kPageText);
  EXPECT_EQ(messages[0].content[2].type,
            ExtendedContentBlockType::kPageExcerpt);
  EXPECT_EQ(messages[0].content[3].type, ExtendedContentBlockType::kText);

  auto* video1 = std::get_if<TextContent>(&messages[0].content[0].data);
  ASSERT_TRUE(video1);
  EXPECT_EQ(video1->text, "Video transcript 1");

  auto* page1 = std::get_if<TextContent>(&messages[0].content[1].data);
  ASSERT_TRUE(page1);
  EXPECT_EQ(page1->text, "Page content 1");

  auto* excerpt1 = std::get_if<TextContent>(&messages[0].content[2].data);
  ASSERT_TRUE(excerpt1);
  EXPECT_EQ(excerpt1->text, "Selected excerpt");

  auto* text1 = std::get_if<TextContent>(&messages[0].content[3].data);
  ASSERT_TRUE(text1);
  EXPECT_EQ(text1->text, "What is this?");

  // Message 2: Assistant turn with no page contents
  EXPECT_EQ(messages[1].role, "assistant");
  ASSERT_EQ(messages[1].content.size(), 1u);
  EXPECT_EQ(messages[1].content[0].type, ExtendedContentBlockType::kText);

  auto* text2 = std::get_if<TextContent>(&messages[1].content[0].data);
  ASSERT_TRUE(text2);
  EXPECT_EQ(text2->text, "This is the answer.");

  // Message 3: Human turn with SUMMARIZE_PAGE action
  EXPECT_EQ(messages[2].role, "user");
  ASSERT_EQ(messages[2].content.size(), 2u);
  EXPECT_EQ(messages[2].content[0].type, ExtendedContentBlockType::kPageText);
  EXPECT_EQ(messages[2].content[1].type,
            ExtendedContentBlockType::kRequestSummary);

  auto* page3 = std::get_if<TextContent>(&messages[2].content[0].data);
  ASSERT_TRUE(page3);
  EXPECT_EQ(page3->text, "Page content 3");

  // Message 4: Human turn with page content, no selected_text
  EXPECT_EQ(messages[3].role, "user");
  ASSERT_EQ(messages[3].content.size(), 2u);
  EXPECT_EQ(messages[3].content[0].type, ExtendedContentBlockType::kPageText);
  EXPECT_EQ(messages[3].content[1].type, ExtendedContentBlockType::kText);

  auto* page4 = std::get_if<TextContent>(&messages[3].content[0].data);
  ASSERT_TRUE(page4);
  EXPECT_EQ(page4->text, "Page content 4");

  auto* text4 = std::get_if<TextContent>(&messages[3].content[1].data);
  ASSERT_TRUE(text4);
  EXPECT_EQ(text4->text, "Another question");
}

TEST_F(OAIMessageUtilsTest, BuildOAIMessages_ContentTruncation) {
  // Create page contents - older content is longer
  PageContent old_content("Old content that will be dropped", false);
  PageContent new_content("New content", false);

  // Build page contents map
  PageContentsMap page_contents_map;
  page_contents_map["turn1"] = {std::cref(old_content)};
  page_contents_map["turn2"] = {std::cref(new_content)};

  // Build conversation history with 2 turns
  EngineConsumer::ConversationHistory history;

  // Turn 1: Older turn with content
  auto turn1 = mojom::ConversationTurn::New();
  turn1->uuid = "turn1";
  turn1->character_type = mojom::CharacterType::HUMAN;
  turn1->action_type = mojom::ActionType::QUERY;
  turn1->text = "First question";
  history.push_back(std::move(turn1));

  // Turn 2: Newer turn with content
  auto turn2 = mojom::ConversationTurn::New();
  turn2->uuid = "turn2";
  turn2->character_type = mojom::CharacterType::HUMAN;
  turn2->action_type = mojom::ActionType::QUERY;
  turn2->text = "Second question";
  history.push_back(std::move(turn2));

  // Set max_length to fit newer content but not both
  std::vector<OAIMessage> messages = BuildOAIMessages(
      std::move(page_contents_map), history, 11, [](std::string&) {});

  // Should have 2 messages
  ASSERT_EQ(messages.size(), 2u);

  // Message 1: Older turn - should have NO page content (dropped)
  EXPECT_EQ(messages[0].role, "user");
  ASSERT_EQ(messages[0].content.size(), 1u);
  EXPECT_EQ(messages[0].content[0].type, ExtendedContentBlockType::kText);

  auto* text1 = std::get_if<TextContent>(&messages[0].content[0].data);
  ASSERT_TRUE(text1);
  EXPECT_EQ(text1->text, "First question");

  // Message 2: Newer turn - should have full page content
  EXPECT_EQ(messages[1].role, "user");
  ASSERT_EQ(messages[1].content.size(), 2u);
  EXPECT_EQ(messages[1].content[0].type, ExtendedContentBlockType::kPageText);
  EXPECT_EQ(messages[1].content[1].type, ExtendedContentBlockType::kText);

  auto* page2 = std::get_if<TextContent>(&messages[1].content[0].data);
  ASSERT_TRUE(page2);
  EXPECT_EQ(page2->text, "New content");

  auto* text2 = std::get_if<TextContent>(&messages[1].content[1].data);
  ASSERT_TRUE(text2);
  EXPECT_EQ(text2->text, "Second question");
}

}  // namespace ai_chat

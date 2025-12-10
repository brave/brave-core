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
#include "brave/components/ai_chat/core/browser/test_utils.h"
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

TEST_F(OAIMessageUtilsTest, BuildOAIQuestionSuggestionsMessages) {
  PageContent text_content1(
      "This is a very long first text content that will be truncated", false);
  PageContent video_content("Short video", true);
  PageContent text_content2("Short text", false);
  PageContents page_contents = {std::cref(text_content1),
                                std::cref(video_content),
                                std::cref(text_content2)};

  bool sanitize_input_called = false;
  std::vector<OAIMessage> messages = BuildOAIQuestionSuggestionsMessages(
      page_contents,
      // Set max length to fit last two blocks fully and truncate the first
      text_content2.content.size() + video_content.content.size() + 2,
      [&sanitize_input_called](std::string&) { sanitize_input_called = true; });

  EXPECT_TRUE(sanitize_input_called);

  // Should return exactly one message
  ASSERT_EQ(messages.size(), 1u);

  const auto& message = messages[0];
  EXPECT_EQ(message.role, "user");

  // Should have 4 blocks: 3 page contents + request questions
  ASSERT_EQ(message.content.size(), 4u);

  // Content is processed in reverse order, so third content comes first
  // Third content (text) should be included in full
  EXPECT_EQ(message.content[0].type, ExtendedContentBlockType::kPageText);
  auto* text2 = std::get_if<TextContent>(&message.content[0].data);
  ASSERT_TRUE(text2);
  EXPECT_EQ(text2->text, "Short text");

  // Second content (video) should be included in full
  EXPECT_EQ(message.content[1].type,
            ExtendedContentBlockType::kVideoTranscript);
  auto* video = std::get_if<TextContent>(&message.content[1].data);
  ASSERT_TRUE(video);
  EXPECT_EQ(video->text, "Short video");

  // First content (text) should be truncated due to max_length
  EXPECT_EQ(message.content[2].type, ExtendedContentBlockType::kPageText);
  auto* text1 = std::get_if<TextContent>(&message.content[2].data);
  ASSERT_TRUE(text1);
  EXPECT_EQ(text1->text, "Th");

  // Last block is request questions
  EXPECT_EQ(message.content[3].type,
            ExtendedContentBlockType::kRequestQuestions);
  auto* request = std::get_if<TextContent>(&message.content[3].data);
  ASSERT_TRUE(request);
  EXPECT_EQ(request->text, "");
}

TEST_F(OAIMessageUtilsTest, BuildOAIGenerateConversationTitleMessages_Basic) {
  // Create a conversation history with 1 human turn and 1 assistant turn
  // without page contents or selected text.
  // Tests one message with only 1 kRequestTitle block with text set to human
  // turn's text is returned.
  auto history = CreateSampleChatHistory(1);

  auto messages = BuildOAIGenerateConversationTitleMessages(
      PageContentsMap(), history, 10000, [](std::string&) {});

  ASSERT_TRUE(messages);
  ASSERT_EQ(messages->size(), 1u);

  const auto& message = messages->at(0);
  EXPECT_EQ(message.role, "user");

  ASSERT_EQ(message.content.size(), 1u);

  // Should only have a request title block with first turn's text.
  EXPECT_EQ(message.content[0].type, ExtendedContentBlockType::kRequestTitle);
  auto* title_text = std::get_if<TextContent>(&message.content[0].data);
  ASSERT_TRUE(title_text);
  EXPECT_EQ(title_text->text, history[0]->text);
}

TEST_F(OAIMessageUtilsTest,
       BuildOAIGenerateConversationTitleMessages_WithExtraContext) {
  // Create a conversation history with 1 human turn with 1 page content and
  // selected text, and 1 assistant turn.
  // Tests one message with 1 page content block, one page excerpt block, and
  // one kRequestTitle block with text set to human turn's text is returned.
  PageContent page_content("Test page content", false);

  auto history = CreateSampleChatHistory(1);
  history[0]->selected_text = "Selected text excerpt";

  PageContentsMap page_contents_map;
  page_contents_map[*history[0]->uuid] = {std::cref(page_content)};

  auto messages = BuildOAIGenerateConversationTitleMessages(
      std::move(page_contents_map), history, 10000, [](std::string&) {});

  ASSERT_TRUE(messages);
  ASSERT_EQ(messages->size(), 1u);

  const auto& message = messages->at(0);
  EXPECT_EQ(message.role, "user");

  ASSERT_EQ(message.content.size(), 3u);

  // First block should be a page text block with page content text.
  EXPECT_EQ(message.content[0].type, ExtendedContentBlockType::kPageText);
  auto* page_text = std::get_if<TextContent>(&message.content[0].data);
  ASSERT_TRUE(page_text);
  EXPECT_EQ(page_text->text, "Test page content");

  // Second block should be a page excerpt block with selected text.
  EXPECT_EQ(message.content[1].type, ExtendedContentBlockType::kPageExcerpt);
  auto* excerpt_text = std::get_if<TextContent>(&message.content[1].data);
  ASSERT_TRUE(excerpt_text);
  EXPECT_EQ(excerpt_text->text, "Selected text excerpt");

  // Third block should be a request title block with first turn's text.
  EXPECT_EQ(message.content[2].type, ExtendedContentBlockType::kRequestTitle);
  auto* title_text = std::get_if<TextContent>(&message.content[2].data);
  ASSERT_TRUE(title_text);
  EXPECT_EQ(title_text->text, history[0]->text);
}

TEST_F(OAIMessageUtilsTest,
       BuildOAIGenerateConversationTitleMessages_UploadFiles) {
  // Create a conversation history with 1 human turn including upload_files,
  // and 1 assistant turn.
  // Tests one message with 1 kRequestTitle block with text set to assistant
  // turn's text is returned.
  PageContentsMap page_contents_map;

  auto history = CreateSampleChatHistory(1);

  auto uploaded_file = mojom::UploadedFile::New();
  uploaded_file->filename = "test.png";
  uploaded_file->filesize = 1024;
  uploaded_file->type = mojom::UploadedFileType::kImage;
  history[0]->uploaded_files.emplace();
  history[0]->uploaded_files->push_back(std::move(uploaded_file));

  auto messages = BuildOAIGenerateConversationTitleMessages(
      std::move(page_contents_map), history, 10000, [](std::string&) {});

  ASSERT_TRUE(messages);
  ASSERT_EQ(messages->size(), 1u);

  const auto& message = messages->at(0);
  EXPECT_EQ(message.role, "user");

  ASSERT_EQ(message.content.size(), 1u);

  // Request title block should use assistant response as the text when there
  // are upload files.
  EXPECT_EQ(message.content[0].type, ExtendedContentBlockType::kRequestTitle);
  auto* title_text = std::get_if<TextContent>(&message.content[0].data);
  ASSERT_TRUE(title_text);
  EXPECT_EQ(title_text->text, history[1]->text);
}

TEST_F(OAIMessageUtilsTest,
       BuildOAIGenerateConversationTitleMessages_ContentTruncation) {
  // Create a converation history with 1 human turn with 4 page content blocks
  // (1 normal, 1 truncated due to max_per_content limit, 1 truncated due to max
  // associated content length, 1 dropped due to max associated content length)
  // and 1 assistant turn.
  // Tests one message with 3 page content blocks and one kRequestTitle block
  // with text set to human turn's text is returned.
  PageContent content1(std::string(1000, 'a'), false);
  PageContent content2(std::string(1000, 'b'), false);
  PageContent content3(std::string(1500, 'c'), false);
  PageContent content4(std::string(500, 'd'), false);

  auto history = CreateSampleChatHistory(1);

  PageContentsMap page_contents_map;
  page_contents_map[*history[0]->uuid] = {
      std::cref(content1), std::cref(content2), std::cref(content3),
      std::cref(content4)};

  auto messages = BuildOAIGenerateConversationTitleMessages(
      std::move(page_contents_map), history, 1800, [](std::string&) {});

  ASSERT_TRUE(messages);
  ASSERT_EQ(messages->size(), 1u);

  const auto& message = messages->at(0);
  EXPECT_EQ(message.role, "user");

  ASSERT_EQ(message.content.size(), 4u);

  // Content 4 (newest): normal, included fully
  EXPECT_EQ(message.content[0].type, ExtendedContentBlockType::kPageText);
  auto* text4 = std::get_if<TextContent>(&message.content[0].data);
  ASSERT_TRUE(text4);
  EXPECT_EQ(text4->text.size(), 500u);
  EXPECT_EQ(text4->text, std::string(500, 'd'));

  // Content 3: truncated to 1200 due to max_per_content limit
  EXPECT_EQ(message.content[1].type, ExtendedContentBlockType::kPageText);
  auto* text3 = std::get_if<TextContent>(&message.content[1].data);
  ASSERT_TRUE(text3);
  EXPECT_EQ(text3->text.size(), 1200u);
  EXPECT_EQ(text3->text, std::string(1200, 'c'));

  // Content 2: truncated to 100 due to remaining_length
  EXPECT_EQ(message.content[2].type, ExtendedContentBlockType::kPageText);
  auto* text2 = std::get_if<TextContent>(&message.content[2].data);
  ASSERT_TRUE(text2);
  EXPECT_EQ(text2->text.size(), 100u);
  EXPECT_EQ(text2->text, std::string(100, 'b'));

  // Content 1 is dropped (not included)

  // kRequestTitle block
  EXPECT_EQ(message.content[3].type, ExtendedContentBlockType::kRequestTitle);
  auto* title_text = std::get_if<TextContent>(&message.content[3].data);
  ASSERT_TRUE(title_text);
  EXPECT_EQ(title_text->text, history[0]->text);
}

TEST_F(OAIMessageUtilsTest,
       BuildOAIGenerateConversationTitleMessages_UnexpectedConversations) {
  // Tests std::nullopt should be returned if the conversation isn't exactly 1
  // human turn and 1 assistant turn.

  // Case 1: Only 1 turn
  {
    auto history = CreateSampleChatHistory(1);
    history.pop_back();  // Remove assistant turn

    auto messages = BuildOAIGenerateConversationTitleMessages(
        PageContentsMap(), history, 10000, [](std::string&) {});

    EXPECT_FALSE(messages);
  }

  // Case 2: 3 turns (1 human + 1 assistant + 1 human)
  {
    auto history = CreateSampleChatHistory(1);

    auto turn3 = mojom::ConversationTurn::New();
    turn3->uuid = "turn3";
    turn3->character_type = mojom::CharacterType::HUMAN;
    turn3->action_type = mojom::ActionType::QUERY;
    turn3->text = "Second question";
    history.push_back(std::move(turn3));

    auto messages = BuildOAIGenerateConversationTitleMessages(
        PageContentsMap(), history, 10000, [](std::string&) {});

    EXPECT_FALSE(messages);
  }
}

}  // namespace ai_chat

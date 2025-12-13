/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/engine/test_utils.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kTestText[] = "This is test text for rewriting.";
constexpr char kSeedText[] = "Here is the rewritten version:";

struct RewriteActionTestParam {
  mojom::ActionType action_type;
  std::optional<mojom::ContentBlock::Tag> expected_content_type;
  std::string expected_payload;  // non-empty for change tones
  std::optional<mojom::SimpleRequestType> expected_simple_request_type;
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
  VerifyPageExcerptBlock(FROM_HERE, message.content[0], kTestText);

  // Second block should be the action-specific content
  ASSERT_EQ(message.content[1]->which(), param.expected_content_type);

  switch (param.expected_content_type.value()) {
    case mojom::ContentBlock::Tag::kChangeToneContentBlock:
      VerifyChangeToneBlock(FROM_HERE, message.content[1], "",
                            param.expected_payload);
      break;
    case mojom::ContentBlock::Tag::kSimpleRequestContentBlock: {
      ASSERT_TRUE(param.expected_simple_request_type.has_value());
      VerifySimpleRequestBlock(FROM_HERE, message.content[1],
                               param.expected_simple_request_type.value());
      break;
    }
    default:
      FAIL() << "Unexpected content block type";
  }
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BuildOAIRewriteSuggestionMessagesTest,
    testing::Values(
        RewriteActionTestParam{
            mojom::ActionType::PARAPHRASE,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock, "",
            mojom::SimpleRequestType::kParaphrase},
        RewriteActionTestParam{
            mojom::ActionType::IMPROVE,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock, "",
            mojom::SimpleRequestType::kImprove},
        RewriteActionTestParam{
            mojom::ActionType::ACADEMICIZE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "academic"},
        RewriteActionTestParam{
            mojom::ActionType::PROFESSIONALIZE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "professional"},
        RewriteActionTestParam{
            mojom::ActionType::PERSUASIVE_TONE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "persuasive"},
        RewriteActionTestParam{
            mojom::ActionType::CASUALIZE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "casual"},
        RewriteActionTestParam{
            mojom::ActionType::FUNNY_TONE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "funny"},
        RewriteActionTestParam{
            mojom::ActionType::SHORTEN,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock, "",
            mojom::SimpleRequestType::kShorten},
        RewriteActionTestParam{
            mojom::ActionType::EXPAND,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock, "",
            mojom::SimpleRequestType::kExpand},
        RewriteActionTestParam{mojom::ActionType::CREATE_TAGLINE, std::nullopt,
                               ""}));

TEST_F(OAIMessageUtilsTest, BuildOAISeedMessage) {
  OAIMessage message = BuildOAISeedMessage(kSeedText);

  EXPECT_EQ(message.role, "assistant");
  ASSERT_EQ(message.content.size(), 1u);
  VerifyTextBlock(FROM_HERE, message.content[0], kSeedText);
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
      std::move(page_contents_map), history, nullptr, true, 10000,
      [&sanitize_input_called](std::string&) { sanitize_input_called = true; });

  EXPECT_TRUE(sanitize_input_called);

  // Should have 4 messages
  ASSERT_EQ(messages.size(), 4u);

  // Message 1: Human turn with all content types
  EXPECT_EQ(messages[0].role, "user");
  ASSERT_EQ(messages[0].content.size(), 4u);
  VerifyVideoTranscriptBlock(FROM_HERE, messages[0].content[0],
                             "Video transcript 1");

  VerifyPageTextBlock(FROM_HERE, messages[0].content[1], "Page content 1");

  VerifyPageExcerptBlock(FROM_HERE, messages[0].content[2], "Selected excerpt");

  VerifyTextBlock(FROM_HERE, messages[0].content[3], "What is this?");

  // Message 2: Assistant turn with no page contents
  EXPECT_EQ(messages[1].role, "assistant");
  ASSERT_EQ(messages[1].content.size(), 1u);
  VerifyTextBlock(FROM_HERE, messages[1].content[0], "This is the answer.");

  // Message 3: Human turn with SUMMARIZE_PAGE action
  EXPECT_EQ(messages[2].role, "user");
  ASSERT_EQ(messages[2].content.size(), 2u);
  VerifyPageTextBlock(FROM_HERE, messages[2].content[0], "Page content 3");

  VerifySimpleRequestBlock(FROM_HERE, messages[2].content[1],
                           mojom::SimpleRequestType::kRequestSummary);

  // Message 4: Human turn with page content, no selected_text
  EXPECT_EQ(messages[3].role, "user");
  ASSERT_EQ(messages[3].content.size(), 2u);
  VerifyPageTextBlock(FROM_HERE, messages[3].content[0], "Page content 4");

  VerifyTextBlock(FROM_HERE, messages[3].content[1], "Another question");
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
  std::vector<OAIMessage> messages =
      BuildOAIMessages(std::move(page_contents_map), history, nullptr, true, 11,
                       [](std::string&) {});

  // Should have 2 messages
  ASSERT_EQ(messages.size(), 2u);

  // Message 1: Older turn - should have NO page content (dropped)
  EXPECT_EQ(messages[0].role, "user");
  ASSERT_EQ(messages[0].content.size(), 1u);
  VerifyTextBlock(FROM_HERE, messages[0].content[0], "First question");

  // Message 2: Newer turn - should have full page content
  EXPECT_EQ(messages[1].role, "user");
  ASSERT_EQ(messages[1].content.size(), 2u);
  VerifyPageTextBlock(FROM_HERE, messages[1].content[0], "New content");

  VerifyTextBlock(FROM_HERE, messages[1].content[1], "Second question");
}

TEST_F(OAIMessageUtilsTest, BuildOAIMessages_UploadedFiles) {
  // Build a conversation history:
  // [0] User message 1: 2 images, no page content or selected text
  // [1] Assistant message 1
  // [2] User message 2: 2 screenshots, with page content, without selected text
  // [3] Assistant message 2
  // [4] User message 3: 2 pdfs, without page content, with selected text
  // [5] Assistant message 3
  // [6] User message 4: 2 images, 2 screenshots, 2 pdfs, without page contents
  // or selected text
  // [7] Assistant message 4
  // [8] User message 5: 2 images, 2 screenshots, 2 pdfs, with page contents and
  // selected text
  // [9] Assistant message 5
  auto history = CreateSampleChatHistory(5);

  // Create uploaded files
  auto images = CreateSampleUploadedFiles(2, mojom::UploadedFileType::kImage);
  auto screenshots =
      CreateSampleUploadedFiles(2, mojom::UploadedFileType::kScreenshot);
  auto pdfs = CreateSampleUploadedFiles(2, mojom::UploadedFileType::kPdf);
  // Clear filename from second PDF to test the default filename behavior
  pdfs[1]->filename.clear();

  auto image_url1 = GURL(EngineConsumer::GetImageDataURL(images[0]->data));
  auto image_url2 = GURL(EngineConsumer::GetImageDataURL(images[1]->data));
  auto screenshot_url1 =
      GURL(EngineConsumer::GetImageDataURL(screenshots[0]->data));
  auto screenshot_url2 =
      GURL(EngineConsumer::GetImageDataURL(screenshots[1]->data));
  auto pdf_url1 = GURL(EngineConsumer::GetPdfDataURL(pdfs[0]->data));
  auto pdf_url2 = GURL(EngineConsumer::GetPdfDataURL(pdfs[1]->data));
  auto pdf_filename1 = pdfs[0]->filename;
  // pdfs[1]->filename was cleared, so it will use default "uploaded.pdf"

  // Build all_files by cloning and combining all file types
  auto images_clone = Clone(images);
  auto screenshots_clone = Clone(screenshots);
  auto pdfs_clone = Clone(pdfs);

  std::vector<mojom::UploadedFilePtr> all_files;
  all_files.insert(all_files.end(),
                   std::make_move_iterator(images_clone.begin()),
                   std::make_move_iterator(images_clone.end()));
  all_files.insert(all_files.end(),
                   std::make_move_iterator(screenshots_clone.begin()),
                   std::make_move_iterator(screenshots_clone.end()));
  all_files.insert(all_files.end(), std::make_move_iterator(pdfs_clone.begin()),
                   std::make_move_iterator(pdfs_clone.end()));

  // Create page contents
  PageContent page_content1("Page content 1", false);
  PageContent page_content2("Page content 2", false);

  PageContentsMap page_contents_map;
  // User message 1: 2 images, no page content or selected text
  history[0]->uploaded_files = Clone(images);
  // User message 2: 2 screenshots, with page content, without selected text
  history[2]->uploaded_files = Clone(screenshots);
  page_contents_map[*history[2]->uuid] = {std::cref(page_content1)};
  // User message 3: 2 pdfs, without page content, with selected text
  history[4]->uploaded_files = Clone(pdfs);
  history[4]->selected_text = "selected_text";
  // User message 4: 2 images, 2 screenshots, 2 pdfs, without page contents or
  // selected text
  history[6]->uploaded_files = Clone(all_files);
  // User message 5: 2 images, 2 screenshots, 2 pdfs, with page contents and
  // selected text
  history[8]->uploaded_files = Clone(all_files);
  history[8]->selected_text = "selected_text";
  page_contents_map[*history[8]->uuid] = {std::cref(page_content2)};

  // Update assistant turn texts for testing
  history[1]->text = "response0";
  history[3]->text = "response1";
  history[5]->text = "response2";
  history[7]->text = "response3";
  history[9]->text = "response4";

  // Build OAI messages
  std::vector<OAIMessage> messages =
      BuildOAIMessages(std::move(page_contents_map), history, nullptr, true,
                       10000, [](std::string&) {});

  // Should have 10 messages (5 human, 5 assistant)
  ASSERT_EQ(messages.size(), 10u);

  // Message 1: Human turn with 2 images
  EXPECT_EQ(messages[0].role, "user");
  // Content: images intro text + 2 images + prompt = 4 blocks
  ASSERT_EQ(messages[0].content.size(), 4u);
  VerifyTextBlock(FROM_HERE, messages[0].content[0],
                  "These images are uploaded by the user");
  VerifyImageBlock(FROM_HERE, messages[0].content[1], image_url1);
  VerifyImageBlock(FROM_HERE, messages[0].content[2], image_url2);
  VerifyTextBlock(FROM_HERE, messages[0].content[3], "query0");

  // Message 2: Assistant turn
  EXPECT_EQ(messages[1].role, "assistant");
  ASSERT_EQ(messages[1].content.size(), 1u);
  VerifyTextBlock(FROM_HERE, messages[1].content[0], "response0");

  // Message 3: Human turn with 2 screenshots, page content
  EXPECT_EQ(messages[2].role, "user");
  // Content: page + screenshots intro + 2 screenshots + prompt = 5 blocks
  ASSERT_EQ(messages[2].content.size(), 5u);
  VerifyPageTextBlock(FROM_HERE, messages[2].content[0], "Page content 1");
  VerifyTextBlock(FROM_HERE, messages[2].content[1],
                  "These images are screenshots");
  VerifyImageBlock(FROM_HERE, messages[2].content[2], screenshot_url1);
  VerifyImageBlock(FROM_HERE, messages[2].content[3], screenshot_url2);
  VerifyTextBlock(FROM_HERE, messages[2].content[4], "query1");

  // Message 4: Assistant turn
  EXPECT_EQ(messages[3].role, "assistant");
  ASSERT_EQ(messages[3].content.size(), 1u);
  VerifyTextBlock(FROM_HERE, messages[3].content[0], "response1");

  // Message 5: Human turn with 2 PDFs and selected text
  EXPECT_EQ(messages[4].role, "user");
  // Content: PDFs intro text + 2 PDFs + selected text + prompt = 5 blocks
  ASSERT_EQ(messages[4].content.size(), 5u);
  VerifyTextBlock(FROM_HERE, messages[4].content[0],
                  "These PDFs are uploaded by the user");
  VerifyFileBlock(FROM_HERE, messages[4].content[1], pdf_url1, pdf_filename1);
  // Second PDF should have default filename since we cleared it
  VerifyFileBlock(FROM_HERE, messages[4].content[2], pdf_url2, "uploaded.pdf");
  VerifyPageExcerptBlock(FROM_HERE, messages[4].content[3], "selected_text");
  VerifyTextBlock(FROM_HERE, messages[4].content[4], "query2");

  // Message 6: Assistant turn
  EXPECT_EQ(messages[5].role, "assistant");
  ASSERT_EQ(messages[5].content.size(), 1u);
  VerifyTextBlock(FROM_HERE, messages[5].content[0], "response2");

  // Message 7: Human turn with all file types, without page content, without
  // selected text
  EXPECT_EQ(messages[6].role, "user");
  // Content: images intro + 2 images + screenshots intro + 2 screenshots +
  // PDFs intro + 2 PDFs + prompt = 10 blocks
  ASSERT_EQ(messages[6].content.size(), 10u);
  VerifyTextBlock(FROM_HERE, messages[6].content[0],
                  "These images are uploaded by the user");
  VerifyImageBlock(FROM_HERE, messages[6].content[1], image_url1);
  VerifyImageBlock(FROM_HERE, messages[6].content[2], image_url2);
  VerifyTextBlock(FROM_HERE, messages[6].content[3],
                  "These images are screenshots");
  VerifyImageBlock(FROM_HERE, messages[6].content[4], screenshot_url1);
  VerifyImageBlock(FROM_HERE, messages[6].content[5], screenshot_url2);
  VerifyTextBlock(FROM_HERE, messages[6].content[6],
                  "These PDFs are uploaded by the user");
  VerifyFileBlock(FROM_HERE, messages[6].content[7], pdf_url1, pdf_filename1);
  VerifyFileBlock(FROM_HERE, messages[6].content[8], pdf_url2, "uploaded.pdf");
  VerifyTextBlock(FROM_HERE, messages[6].content[9], "query3");

  // Message 8: Assistant turn
  EXPECT_EQ(messages[7].role, "assistant");
  ASSERT_EQ(messages[7].content.size(), 1u);
  VerifyTextBlock(FROM_HERE, messages[7].content[0], "response3");

  // Message 9: Human turn with all file types, page content, selected text
  EXPECT_EQ(messages[8].role, "user");
  // Content: page + images intro + 2 images + screenshots intro +
  // 2 screenshots + PDFs intro + 2 PDFs + selected text + prompt = 12 blocks
  ASSERT_EQ(messages[8].content.size(), 12u);
  VerifyPageTextBlock(FROM_HERE, messages[8].content[0], "Page content 2");
  VerifyTextBlock(FROM_HERE, messages[8].content[1],
                  "These images are uploaded by the user");
  VerifyImageBlock(FROM_HERE, messages[8].content[2], image_url1);
  VerifyImageBlock(FROM_HERE, messages[8].content[3], image_url2);
  VerifyTextBlock(FROM_HERE, messages[8].content[4],
                  "These images are screenshots");
  VerifyImageBlock(FROM_HERE, messages[8].content[5], screenshot_url1);
  VerifyImageBlock(FROM_HERE, messages[8].content[6], screenshot_url2);
  VerifyTextBlock(FROM_HERE, messages[8].content[7],
                  "These PDFs are uploaded by the user");
  VerifyFileBlock(FROM_HERE, messages[8].content[8], pdf_url1, pdf_filename1);
  VerifyFileBlock(FROM_HERE, messages[8].content[9], pdf_url2, "uploaded.pdf");
  VerifyPageExcerptBlock(FROM_HERE, messages[8].content[10], "selected_text");
  VerifyTextBlock(FROM_HERE, messages[8].content[11], "query4");

  // Message 10: Assistant turn
  EXPECT_EQ(messages[9].role, "assistant");
  ASSERT_EQ(messages[9].content.size(), 1u);
  VerifyTextBlock(FROM_HERE, messages[9].content[0], "response4");
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
  VerifyPageTextBlock(FROM_HERE, message.content[0], "Short text");

  // Second content (video) should be included in full
  VerifyVideoTranscriptBlock(FROM_HERE, message.content[1], "Short video");

  // First content (text) should be truncated due to max_length
  VerifyPageTextBlock(FROM_HERE, message.content[2], "Th");

  // Last block is request questions
  VerifySimpleRequestBlock(FROM_HERE, message.content[3],
                           mojom::SimpleRequestType::kRequestQuestions);
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
  VerifyRequestTitleBlock(FROM_HERE, message.content[0], history[0]->text);
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
  VerifyPageTextBlock(FROM_HERE, message.content[0], "Test page content");

  // Second block should be a page excerpt block with selected text.
  VerifyPageExcerptBlock(FROM_HERE, message.content[1],
                         "Selected text excerpt");

  // Third block should be a request title block with first turn's text.
  VerifyRequestTitleBlock(FROM_HERE, message.content[2], history[0]->text);
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
  VerifyRequestTitleBlock(FROM_HERE, message.content[0], history[1]->text);
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
  VerifyPageTextBlock(FROM_HERE, message.content[0], std::string(500, 'd'));

  // Content 3: truncated to 1200 due to max_per_content limit
  VerifyPageTextBlock(FROM_HERE, message.content[1], std::string(1200, 'c'));

  // Content 2: truncated to 100 due to remaining_length
  VerifyPageTextBlock(FROM_HERE, message.content[2], std::string(100, 'b'));

  // Content 1 is dropped (not included)

  // kRequestTitle block
  VerifyRequestTitleBlock(FROM_HERE, message.content[3], history[0]->text);
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

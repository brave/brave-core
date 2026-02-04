/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api_v2.h"

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_v2_client.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/test_utils.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using ::testing::_;

namespace ai_chat {

namespace {

constexpr int kTestingMaxAssociatedContentLength = 100;

struct GenerateRewriteTestParam {
  std::string name;
  mojom::ActionType action_type;
  mojom::ContentBlock::Tag expected_content_type;
  std::string expected_payload;
  std::string expected_type_string;
  std::optional<mojom::SimpleRequestType> expected_simple_request_type;
};

}  // namespace

class MockConversationAPIV2Client : public ConversationAPIV2Client {
 public:
  explicit MockConversationAPIV2Client(const std::string& model_name)
      : ConversationAPIV2Client(model_name, nullptr, nullptr, nullptr) {}
  ~MockConversationAPIV2Client() override = default;

  MOCK_METHOD(void,
              PerformRequest,
              (std::vector<OAIMessage>,
               std::optional<base::Value::List> oai_tool_definitions,
               const std::optional<std::string>& preferred_tool_name,
               mojom::ConversationCapability conversation_capability,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback,
               const std::optional<std::string>& model_name),
              (override));

  std::string GetMessagesJson(std::vector<OAIMessage> messages) {
    auto body = CreateJSONRequestBody(
        std::move(messages), std::nullopt, std::nullopt,
        mojom::ConversationCapability::CHAT, std::nullopt, true);
    auto dict = base::test::ParseJsonDict(body);
    base::Value::List* messages_list = dict.FindList("messages");
    EXPECT_TRUE(messages_list);
    std::string messages_json;
    base::JSONWriter::WriteWithOptions(
        *messages_list, base::JSONWriter::OPTIONS_PRETTY_PRINT, &messages_json);
    return messages_json;
  }
};

class EngineConsumerConversationAPIV2UnitTest : public testing::Test {
 public:
  EngineConsumerConversationAPIV2UnitTest() = default;
  ~EngineConsumerConversationAPIV2UnitTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(prefs_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());
    model_service_ = std::make_unique<ModelService>(&prefs_);

    auto options = mojom::LeoModelOptions::New();
    options->display_maker = "Test Maker";
    options->name = "test-model-name";
    options->category = mojom::ModelCategory::CHAT;
    options->access = mojom::ModelAccess::BASIC;
    options->max_associated_content_length = kTestingMaxAssociatedContentLength;
    options->long_conversation_warning_character_limit = 1000;

    model_ = mojom::Model::New();
    model_->key = "test_model_key";
    model_->display_name = "Test Model Display Name";
    model_->options =
        mojom::ModelOptions::NewLeoModelOptions(std::move(options));

    engine_ = std::make_unique<EngineConsumerConversationAPIV2>(
        *model_->options->get_leo_model_options(), nullptr, nullptr,
        model_service_.get(), &prefs_);
    engine_->SetAPIForTesting(std::make_unique<MockConversationAPIV2Client>(
        model_->options->get_leo_model_options()->name));
  }

  MockConversationAPIV2Client* GetMockConversationAPIV2Client() {
    return static_cast<MockConversationAPIV2Client*>(
        engine_->GetAPIForTesting());
  }

  void TearDown() override {}

  std::string FormatComparableMessagesJson(std::string_view formatted_json) {
    auto messages = base::test::ParseJson(formatted_json);
    std::string messages_json;
    base::JSONWriter::WriteWithOptions(
        messages, base::JSONWriter::OPTIONS_PRETTY_PRINT, &messages_json);
    return messages_json;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  mojom::ModelPtr model_;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<EngineConsumerConversationAPIV2> engine_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_BasicMessage) {
  // Although these tests should likely only be testing the
  // EngineConsumerConversationAPI class, we also include testing some
  // functionality of the very related ConversationAPIClient class. Whilst
  // EngineConsumerConversationAPI merely converts from AI Chat schemas
  // such as mojom::ConversationTurn, to OAIMessage, the ConversationAPIClient
  // class also converts from OAIMessage to JSON. It's convenient to test both
  // here but more exhaustive tests of ConversationAPIClient are performed in
  // its own unit test suite.
  PageContent page_content(
      std::string(kTestingMaxAssociatedContentLength + 1, 'a'), false);
  std::string expected_page_content(kTestingMaxAssociatedContentLength, 'a');
  std::string expected_user_message_content =
      "Tell the user which show is this about?";

  // Build expected JSON format
  std::string expected_messages = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {"type": "brave-page-text", "text": "%s"},
            {"type": "text", "text": "%s"}
          ]
        }
      ])",
      expected_page_content, expected_user_message_content);

  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 1 message with 2 content blocks
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 2u);

        // First content block should be page text
        // Page content should be truncated
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            expected_page_content);

        // Second content block should be the user message
        VerifyTextBlock(FROM_HERE, messages[0].content[1],
                        expected_user_message_content);

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = "Which show is this about?";
  turn->prompt = "Tell the user which show is this about?";
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content}}}}, history, false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_BasicMessage_MultiAssociatedTruncates) {
  size_t content_length = kTestingMaxAssociatedContentLength / 2 + 10;
  PageContent page_content_1(std::string(content_length, 'a'), false);
  PageContent page_content_2(std::string(content_length, 'b'), false);
  // First content should be truncated to remaining available space (as we
  // truncate the oldest page content first).
  std::string expected_page_content_1(
      kTestingMaxAssociatedContentLength - content_length, 'a');
  std::string expected_page_content_2(content_length, 'b');

  std::string expected_user_message_content =
      "Tell the user which show is this about?";

  // Build expected JSON format
  std::string expected_messages = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {"type": "brave-page-text", "text": "%s"},
            {"type": "brave-page-text", "text": "%s"},
            {"type": "text", "text": "%s"}
          ]
        }
      ])",
      expected_page_content_2, expected_page_content_1,
      expected_user_message_content);

  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should be 1 message with 3 content blocks (2 page texts + user
        // message)
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 3u);

        // Content blocks should be ordered: newer page content first
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            expected_page_content_2);

        VerifyPageTextBlock(FROM_HERE, messages[0].content[1],
                            expected_page_content_1);

        VerifyTextBlock(FROM_HERE, messages[0].content[2],
                        expected_user_message_content);

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = "Which show is this about?";
  turn->prompt = "Tell the user which show is this about?";
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content_1, page_content_2}}}}, history, false, {},
      std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithSelectedText) {
  PageContent page_content("This is a page about The Mandalorian.", false);

  // Build expected JSON format
  std::string expected_messages = R"([
    {
      "role": "user",
      "content": [
        {"type": "brave-page-text",
         "text": "This is a page about The Mandalorian."},
        {"type": "brave-page-excerpt", "text": "The Mandalorian"},
        {"type": "text", "text": "Is this related to a broader series?"}
      ]
    }
  ])";

  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should be 1 message with 3 blocks (page, excerpt, text)
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 3u);

        // Page content
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            "This is a page about The Mandalorian.");

        // Selected text (page excerpt)
        VerifyPageExcerptBlock(FROM_HERE, messages[0].content[1],
                               "The Mandalorian");

        // User message
        VerifyTextBlock(FROM_HERE, messages[0].content[2],
                        "Is this related to a broader series?");

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = "Is this related to a broader series?";
  turn->selected_text = "The Mandalorian";
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content}}}}, history, false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_HistoryWithSelectedText) {
  PageContent page_content("This is my page. I have spoken.", false);
  // Tests messages building from history with selected text and new query
  // without selected text but with page association.
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Which show is this catchphrase from?", std::nullopt /* prompt */,
      "I have spoken.", std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "The Mandalorian.", std::nullopt /* prompt */, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt /* model_key */, nullptr /* near_verification_status */));
  history.push_back(mojom::ConversationTurn::New(
      "turn-3", mojom::CharacterType::HUMAN, mojom::ActionType::RESPONSE,
      "Is it related to a broader series?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  // Build expected JSON format
  std::string expected_messages = R"([
    {
      "role": "user",
      "content": [
        {"type": "brave-page-text",
         "text": "This is my page. I have spoken."},
        {"type": "brave-page-excerpt", "text": "I have spoken."},
        {"type": "text", "text": "Which show is this catchphrase from?"}
      ]
    },
    {
      "role": "assistant",
      "content": [
        {"type": "text", "text": "The Mandalorian."}
      ]
    },
    {
      "role": "user",
      "content": [
        {"type": "text", "text": "Is it related to a broader series?"}
      ]
    }
  ])";

  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 3 messages (user, assistant, user)
        EXPECT_EQ(messages.size(), 3u);

        // First message: user with page content, excerpt, and text
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 3u);
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            "This is my page. I have spoken.");
        VerifyPageExcerptBlock(FROM_HERE, messages[0].content[1],
                               "I have spoken.");
        VerifyTextBlock(FROM_HERE, messages[0].content[2],
                        "Which show is this catchphrase from?");

        // Second message: assistant response
        EXPECT_EQ(messages[1].role, "assistant");
        ASSERT_EQ(messages[1].content.size(), 1u);
        VerifyTextBlock(FROM_HERE, messages[1].content[0], "The Mandalorian.");

        // Third message: user follow-up
        EXPECT_EQ(messages[2].role, "user");
        ASSERT_EQ(messages[2].content.size(), 1u);
        VerifyTextBlock(FROM_HERE, messages[2].content[0],
                        "Is it related to a broader series?");

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content}}}}, history, false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_ModifyReply) {
  // Tests messages building from history with modified agent reply.
  EngineConsumer::ConversationHistory history;
  PageContent page_content("I have spoken.", false);
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Which show is 'This is the way' from?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  std::vector<mojom::ConversationEntryEventPtr> events;
  auto search_event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
      mojom::SearchStatusEvent::New());
  auto completion_event = mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("Mandalorian"));
  events.push_back(search_event.Clone());
  events.push_back(completion_event.Clone());

  std::vector<mojom::ConversationEntryEventPtr> modified_events;
  modified_events.push_back(search_event.Clone());
  auto modified_completion_event =
      mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New("The Mandalorian"));
  modified_events.push_back(modified_completion_event.Clone());

  auto edit = mojom::ConversationTurn::New(
      "edit-1", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "The Mandalorian.", std::nullopt /* prompt */, std::nullopt,
      std::move(modified_events), base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */);
  std::vector<mojom::ConversationTurnPtr> edits;
  edits.push_back(std::move(edit));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Mandalorian.", std::nullopt /* prompt */, std::nullopt,
      std::move(events), base::Time::Now(), std::move(edits), std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));
  history.push_back(mojom::ConversationTurn::New(
      "turn-3", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Is it related to a broader series?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  // Build expected JSON format
  std::string expected_messages = R"([
    {
      "role": "user",
      "content": [
        {"type": "brave-page-text", "text": "I have spoken."},
        {"type": "text", "text": "Which show is 'This is the way' from?"}
      ]
    },
    {
      "role": "assistant",
      "content": [
        {"type": "text", "text": "The Mandalorian."}
      ]
    },
    {
      "role": "user",
      "content": [
        {"type": "text", "text": "Is it related to a broader series?"}
      ]
    }
  ])";

  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 3 messages (user with page, assistant, user)
        ASSERT_EQ(messages.size(), 3u);

        // First message: user with page content
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 2u);
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            "I have spoken.");
        VerifyTextBlock(FROM_HERE, messages[0].content[1],
                        "Which show is 'This is the way' from?");

        // Second message: assistant (modified reply)
        EXPECT_EQ(messages[1].role, "assistant");
        ASSERT_EQ(messages[1].content.size(), 1u);
        VerifyTextBlock(FROM_HERE, messages[1].content[0], "The Mandalorian.");

        // Third message: user follow-up
        EXPECT_EQ(messages[2].role, "user");
        ASSERT_EQ(messages[2].content.size(), 1u);
        VerifyTextBlock(FROM_HERE, messages[2].content[0],
                        "Is it related to a broader series?");

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content}}}}, history, false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_SummarizePage) {
  // Build expected JSON format
  std::string expected_messages = R"([
    {
      "role": "user",
      "content": [
        {"type": "brave-page-text",
         "text": "This is a sample page content."},
        {"type": "brave-request-summary", "text": ""}
      ]
    }
  ])";

  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 1 message with 2 blocks (page text, request summary)
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 2u);

        // Page content block
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            "This is a sample page content.");

        // Request summary block
        VerifySimpleRequestBlock(FROM_HERE, messages[0].content[1],
                                 mojom::SimpleRequestType::kRequestSummary);

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->action_type = mojom::ActionType::SUMMARIZE_PAGE;
  // This text should be ignored
  turn->text = "Summarize the content of this page.";
  history.push_back(std::move(turn));
  PageContent page_content("This is a sample page content.", false);

  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content}}}}, history, false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithMemoryBlock) {
  auto* mock_api_client = GetMockConversationAPIV2Client();

  // Test with user customization enabled
  {
    prefs_.SetBoolean(prefs::kBraveAIChatUserCustomizationEnabled, true);

    base::Value::Dict customizations_dict;
    customizations_dict.Set("name", "John Doe");
    customizations_dict.Set("job", "Software Engineer");
    customizations_dict.Set("tone", "Professional");
    customizations_dict.Set("other", "Loves coding");
    prefs_.SetDict(prefs::kBraveAIChatUserCustomizations,
                   std::move(customizations_dict));

    prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);

    std::string expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {
                "type": "brave-user-memory",
                "memory": {
                  "job": "Software Engineer",
                  "name": "John Doe",
                  "other": "Loves coding",
                  "tone": "Professional"
                }
              },
              {"type": "brave-page-text", "text": "%s"},
              {"type": "text", "text": "%s"}
            ]
          }
        ])",
        "This is a test page content.", "What is this about?");

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          ASSERT_EQ(messages.size(), 1u);
          EXPECT_EQ(messages[0].role, "user");
          ASSERT_EQ(messages[0].content.size(), 3u);

          auto expected_memory =
              BuildExpectedMemory({{"job", "Software Engineer"},
                                   {"name", "John Doe"},
                                   {"other", "Loves coding"},
                                   {"tone", "Professional"}},
                                  {});
          VerifyMemoryBlock(FROM_HERE, messages[0].content[0], expected_memory);
          VerifyPageTextBlock(FROM_HERE, messages[0].content[1],
                              "This is a test page content.");
          VerifyTextBlock(FROM_HERE, messages[0].content[2],
                          "What is this about?");

          EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                    FormatComparableMessagesJson(expected_messages));
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Test response"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    std::vector<mojom::ConversationTurnPtr> history;
    mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
    turn->uuid = "turn-1";
    turn->character_type = mojom::CharacterType::HUMAN;
    turn->text = "What is this about?";
    history.push_back(std::move(turn));

    base::RunLoop run_loop;
    PageContent page_content("This is a test page content.", false);
    engine_->GenerateAssistantResponse(
        {{"turn-1", {page_content}}}, std::move(history), false, {},
        std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::GenerationResult) {
              run_loop.Quit();
            }));
    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test with user memory enabled
  {
    prefs_.SetBoolean(prefs::kBraveAIChatUserCustomizationEnabled, false);
    prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);

    base::Value::List memories;
    memories.Append("I prefer concise explanations");
    memories.Append("I work in the tech industry");
    prefs_.SetList(prefs::kBraveAIChatUserMemories, std::move(memories));

    std::string expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {
                "type": "brave-user-memory",
                "memory": {
                  "memories": [
                    "I prefer concise explanations",
                    "I work in the tech industry"
                  ]
                }
              },
              {"type": "brave-page-text", "text": "%s"},
              {"type": "text", "text": "%s"}
            ]
          }
        ])",
        "This is a test page content.", "What is this about?");

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          ASSERT_EQ(messages.size(), 1u);
          EXPECT_EQ(messages[0].role, "user");
          ASSERT_EQ(messages[0].content.size(), 3u);

          auto expected_memory =
              BuildExpectedMemory({}, {{"memories",
                                        {"I prefer concise explanations",
                                         "I work in the tech industry"}}});
          VerifyMemoryBlock(FROM_HERE, messages[0].content[0], expected_memory);
          VerifyPageTextBlock(FROM_HERE, messages[0].content[1],
                              "This is a test page content.");
          VerifyTextBlock(FROM_HERE, messages[0].content[2],
                          "What is this about?");

          EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                    FormatComparableMessagesJson(expected_messages));
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Test response"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    std::vector<mojom::ConversationTurnPtr> history;
    mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
    turn->uuid = "turn-1";
    turn->character_type = mojom::CharacterType::HUMAN;
    turn->text = "What is this about?";
    history.push_back(std::move(turn));

    base::RunLoop run_loop;
    PageContent page_content("This is a test page content.", false);
    engine_->GenerateAssistantResponse(
        {{"turn-1", {page_content}}}, std::move(history), false, {},
        std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::GenerationResult) {
              run_loop.Quit();
            }));
    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test with both customization and memory enabled
  {
    prefs_.SetBoolean(prefs::kBraveAIChatUserCustomizationEnabled, true);
    prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);

    base::Value::Dict customizations_dict;
    customizations_dict.Set("name", "Alice");
    customizations_dict.Set("job", "Designer");
    prefs_.SetDict(prefs::kBraveAIChatUserCustomizations,
                   std::move(customizations_dict));

    base::Value::List memories;
    memories.Append("I like creative solutions");
    prefs_.SetList(prefs::kBraveAIChatUserMemories, std::move(memories));

    std::string expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {
                "type": "brave-user-memory",
                "memory": {
                  "job": "Designer",
                  "memories": ["I like creative solutions"],
                  "name": "Alice"
                }
              },
              {"type": "brave-page-text", "text": "%s"},
              {"type": "text", "text": "%s"}
            ]
          }
        ])",
        "This is a test page content.", "What is this about?");

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          ASSERT_EQ(messages.size(), 1u);
          EXPECT_EQ(messages[0].role, "user");
          ASSERT_EQ(messages[0].content.size(), 3u);

          auto expected_memory = BuildExpectedMemory(
              {{"job", "Designer"}, {"name", "Alice"}},
              {{"memories", {"I like creative solutions"}}});
          VerifyMemoryBlock(FROM_HERE, messages[0].content[0], expected_memory);
          VerifyPageTextBlock(FROM_HERE, messages[0].content[1],
                              "This is a test page content.");
          VerifyTextBlock(FROM_HERE, messages[0].content[2],
                          "What is this about?");

          EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                    FormatComparableMessagesJson(expected_messages));
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Test response"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    std::vector<mojom::ConversationTurnPtr> history;
    mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
    turn->uuid = "turn-1";
    turn->character_type = mojom::CharacterType::HUMAN;
    turn->text = "What is this about?";
    history.push_back(std::move(turn));

    base::RunLoop run_loop;
    PageContent page_content("This is a test page content.", false);
    engine_->GenerateAssistantResponse(
        {{"turn-1", {page_content}}}, std::move(history), false, {},
        std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::GenerationResult) {
              run_loop.Quit();
            }));
    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test with both customization and memory disabled
  {
    prefs_.SetBoolean(prefs::kBraveAIChatUserCustomizationEnabled, false);
    prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);

    std::string expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {"type": "brave-page-text", "text": "%s"},
              {"type": "text", "text": "%s"}
            ]
          }
        ])",
        "This is a test page content.", "What is this about?");

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          ASSERT_EQ(messages.size(), 1u);
          EXPECT_EQ(messages[0].role, "user");
          ASSERT_EQ(messages[0].content.size(), 2u);

          VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                              "This is a test page content.");
          VerifyTextBlock(FROM_HERE, messages[0].content[1],
                          "What is this about?");

          EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                    FormatComparableMessagesJson(expected_messages));
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Test response"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    std::vector<mojom::ConversationTurnPtr> history;
    mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
    turn->uuid = "turn-1";
    turn->character_type = mojom::CharacterType::HUMAN;
    turn->text = "What is this about?";
    history.push_back(std::move(turn));

    base::RunLoop run_loop;
    PageContent page_content("This is a test page content.", false);
    engine_->GenerateAssistantResponse(
        {{"turn-1", {page_content}}}, std::move(history), false, {},
        std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::GenerationResult) {
              run_loop.Quit();
            }));
    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test with customization enabled but empty values
  {
    prefs_.SetBoolean(prefs::kBraveAIChatUserCustomizationEnabled, true);
    prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);

    // Set empty customizations dict
    base::Value::Dict empty_customizations_dict;
    prefs_.SetDict(prefs::kBraveAIChatUserCustomizations,
                   std::move(empty_customizations_dict));

    std::string expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {"type": "brave-page-text", "text": "%s"},
              {"type": "text", "text": "%s"}
            ]
          }
        ])",
        "This is a test page content.", "What is this about?");

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          ASSERT_EQ(messages.size(), 1u);
          EXPECT_EQ(messages[0].role, "user");
          ASSERT_EQ(messages[0].content.size(), 2u);

          VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                              "This is a test page content.");
          VerifyTextBlock(FROM_HERE, messages[0].content[1],
                          "What is this about?");

          EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                    FormatComparableMessagesJson(expected_messages));
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Test response"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    std::vector<mojom::ConversationTurnPtr> history;
    mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
    turn->uuid = "turn-1";
    turn->character_type = mojom::CharacterType::HUMAN;
    turn->text = "What is this about?";
    history.push_back(std::move(turn));

    base::RunLoop run_loop;
    PageContent page_content("This is a test page content.", false);
    engine_->GenerateAssistantResponse(
        {{"turn-1", {page_content}}}, std::move(history), false, {},
        std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::GenerationResult) {
              run_loop.Quit();
            }));
    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test with memory enabled but empty values
  {
    prefs_.SetBoolean(prefs::kBraveAIChatUserCustomizationEnabled, false);
    prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);

    // Set empty memories list
    base::Value::List empty_memories;
    prefs_.SetList(prefs::kBraveAIChatUserMemories, std::move(empty_memories));

    std::string expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {"type": "brave-page-text", "text": "%s"},
              {"type": "text", "text": "%s"}
            ]
          }
        ])",
        "This is a test page content.", "What is this about?");

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          ASSERT_EQ(messages.size(), 1u);
          EXPECT_EQ(messages[0].role, "user");
          ASSERT_EQ(messages[0].content.size(), 2u);

          VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                              "This is a test page content.");
          VerifyTextBlock(FROM_HERE, messages[0].content[1],
                          "What is this about?");

          EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                    FormatComparableMessagesJson(expected_messages));
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("Test response"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    std::vector<mojom::ConversationTurnPtr> history;
    mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
    turn->uuid = "turn-1";
    turn->character_type = mojom::CharacterType::HUMAN;
    turn->text = "What is this about?";
    history.push_back(std::move(turn));

    base::RunLoop run_loop;
    PageContent page_content("This is a test page content.", false);
    engine_->GenerateAssistantResponse(
        {{"turn-1", {page_content}}}, std::move(history), false, {},
        std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::GenerationResult) {
              run_loop.Quit();
            }));
    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_TemporaryChatExcludesMemory) {
  auto* mock_api_client = GetMockConversationAPIV2Client();

  // Setup user memory to ensure it's available but should be excluded
  prefs_.SetBoolean(prefs::kBraveAIChatUserCustomizationEnabled, true);
  base::Value::Dict customizations_dict;
  customizations_dict.Set("name", "John Doe");
  customizations_dict.Set("job", "Software Engineer");
  prefs_.SetDict(prefs::kBraveAIChatUserCustomizations,
                 std::move(customizations_dict));

  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);
  base::Value::List memories;
  memories.Append("I prefer concise explanations");
  memories.Append("I work in the tech industry");
  prefs_.SetList(prefs::kBraveAIChatUserMemories, std::move(memories));

  // Expect NO memory content block when is_temporary_chat=true
  std::string expected_messages = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {"type": "brave-page-text", "text": "%s"},
            {"type": "text", "text": "%s"}
          ]
        }
      ])",
      "This is a test page content.", "What is this about?");

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should only have 1 message with 2 content blocks
        // NO memory content block should be present
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 2u);

        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            "This is a test page content.");
        VerifyTextBlock(FROM_HERE, messages[0].content[1],
                        "What is this about?");

        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("Test response"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = "What is this about?";
  history.push_back(std::move(turn));

  base::RunLoop run_loop;
  PageContent page_content("This is a test page content.", false);
  engine_->GenerateAssistantResponse(
      {{"turn-1", {page_content}}}, std::move(history),
      true,  // is_temporary_chat = true
      {}, std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithModelKeyOverride) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  constexpr char kModelKey[] = "chat-basic";

  // Expect PerformRequest with the overridden model name
  EXPECT_CALL(
      *mock_api_client,
      PerformRequest(_, _, _, _, _, _,
                     testing::Eq(std::optional<std::string>(
                         model_service_->GetLeoModelNameByKey(kModelKey)))))
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("Test response"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = "What is this about?";
  turn->model_key = kModelKey;
  history.push_back(std::move(turn));

  base::RunLoop run_loop;
  PageContent page_content("This is a test page content.", false);
  PageContentsMap page_contents{{"turn-1", {page_content}}};

  engine_->GenerateAssistantResponse(
      std::move(page_contents), std::move(history), false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithEmptyToolDefinitions) {
  // Verify we're not passing tools if we don't have any
  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client,
              PerformRequest(_, testing::Eq(std::nullopt), _, _, _, _, _))
      .WillOnce([&]() { run_loop.Quit(); });

  auto history = CreateSampleChatHistory(2);

  engine_->GenerateAssistantResponse(
      {}, std::move(history), false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithToolDefinitions) {
  // Verify we're passing json-converted tool definitions.
  // For more variation tests, see oai_parsing_unittest.cc
  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;

  base::Value::Dict properties;
  properties.Set("location", StringProperty("The location to get weather for"));
  properties.Set("units", StringProperty("Temperature units"));

  std::vector<std::string> required_props = {"location"};
  auto mock_tool = std::make_unique<MockTool>(
      "weather_tool", "Get weather", "", std::move(properties), required_props);

  std::string expected_tools_json = R"([
    {
      "type": "function",
      "function": {
        "description": "Get weather",
        "name": "weather_tool",
        "parameters": {
          "type": "object",
          "properties": {
            "location": {
              "type": "string",
              "description": "The location to get weather for"
            },
            "units": {
              "type": "string",
              "description": "Temperature units"
            }
          },
          "required": ["location"]
        }
      }
    }
  ])";

  EXPECT_CALL(*mock_api_client,
              PerformRequest(
                  _, testing::Optional(base::test::IsJson(expected_tools_json)),
                  _, _, _, _, _))
      .WillOnce([&]() { run_loop.Quit(); });

  auto history = CreateSampleChatHistory(2);

  engine_->GenerateAssistantResponse(
      {}, std::move(history), false, {mock_tool->GetWeakPtr()}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       ShouldCallSanitizeInputOnPageContent) {
  class MockConversationAPIEngineConsumer
      : public EngineConsumerConversationAPIV2 {
   public:
    using EngineConsumerConversationAPIV2::EngineConsumerConversationAPIV2;
    ~MockConversationAPIEngineConsumer() override = default;

    MOCK_METHOD(void, SanitizeInput, (std::string & input), (override));
  };

  PageContent page_content_1("This is a page about The Mandalorian.", false);
  PageContent page_content_2("This is a video about The Mandalorian.", true);

  auto mock_engine_consumer =
      std::make_unique<MockConversationAPIEngineConsumer>(
          *model_->options->get_leo_model_options(), nullptr, nullptr,
          model_service_.get(), &prefs_);
  mock_engine_consumer->SetAPIForTesting(
      std::make_unique<MockConversationAPIV2Client>(
          model_->options->get_leo_model_options()->name));

  // Calling GenerateAssistantResponse should call SanitizeInput
  {
    EXPECT_CALL(*mock_engine_consumer, SanitizeInput(page_content_1.content));
    EXPECT_CALL(*mock_engine_consumer, SanitizeInput(page_content_2.content));

    std::vector<mojom::ConversationTurnPtr> history;
    auto turn = mojom::ConversationTurn::New();
    turn->uuid = "turn-1";
    history.push_back(std::move(turn));
    mock_engine_consumer->GenerateAssistantResponse(
        {{{"turn-1", {page_content_1, page_content_2}}}}, history, false, {},
        std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
        base::DoNothing());
    testing::Mock::VerifyAndClearExpectations(mock_engine_consumer.get());
  }

  // Calling GenerateQuestionSuggestions should call SanitizeInput
  {
    EXPECT_CALL(*mock_engine_consumer, SanitizeInput(page_content_1.content));
    EXPECT_CALL(*mock_engine_consumer, SanitizeInput(page_content_2.content));

    mock_engine_consumer->GenerateQuestionSuggestions(
        {page_content_1, page_content_2}, base::DoNothing());
    testing::Mock::VerifyAndClearExpectations(mock_engine_consumer.get());
  }
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_PageContentsOrderedBeforeTurns) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 1 message with content blocks ordered (page content
        // before text)
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_GE(messages[0].content.size(), 2u);

        // First content block should be page content
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            "Test page content");

        // Second content block should be the user message
        VerifyTextBlock(FROM_HERE, messages[0].content[1], "Human message");

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
        run_loop.Quit();
      });

  PageContent page_content("Test page content", false);

  std::vector<mojom::ConversationTurnPtr> history;
  auto turn = mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt, nullptr);
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {{"turn-1", {page_content}}}, std::move(history), false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&](EngineConsumer::GenerationResult) { /* handled above */ }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_PageContentsExcludedForMissingTurns) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should only have user message, no page content for missing turn
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 1u);

        VerifyTextBlock(FROM_HERE, messages[0].content[0], "Human message");

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
        run_loop.Quit();
      });

  // Create page content for a turn UUID that doesn't exist in conversation
  // history
  PageContent page_content("Content for missing turn", false);

  std::vector<mojom::ConversationTurnPtr> history;
  auto turn = mojom::ConversationTurn::New(
      "existing-turn", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt, nullptr);
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {{"missing-turn", {page_content}}}, std::move(history), false, {},
      std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&](EngineConsumer::GenerationResult) { /* handled above */ }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_MultiplePageContentsForSameTurn) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 1 message with 3 content blocks (video, page, text)
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_GE(messages[0].content.size(), 3u);

        // First content block should be video content
        VerifyVideoTranscriptBlock(FROM_HERE, messages[0].content[0],
                                   "Video content");

        // Second content block should be page content
        VerifyPageTextBlock(FROM_HERE, messages[0].content[1],
                            "First page content");

        // Third content block should be the user message
        VerifyTextBlock(FROM_HERE, messages[0].content[2], "Human message");

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
        run_loop.Quit();
      });

  PageContent page_content1("First page content", false);
  PageContent video_content("Video content", true);

  std::vector<mojom::ConversationTurnPtr> history;
  auto turn = mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt, nullptr);
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {{"turn-1", {page_content1, video_content}}}, std::move(history), false,
      {}, std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&](EngineConsumer::GenerationResult) { /* handled above */ }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_MultiTurnConversationWithPageContents) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 3 messages: User, Assistant, User
        ASSERT_EQ(messages.size(), 3u);

        // First message: user with page content for turn-1
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 2u);
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            "Content for first turn");
        VerifyTextBlock(FROM_HERE, messages[0].content[1],
                        "First human message");

        // Second message: assistant response
        EXPECT_EQ(messages[1].role, "assistant");
        ASSERT_EQ(messages[1].content.size(), 1u);
        VerifyTextBlock(FROM_HERE, messages[1].content[0],
                        "First assistant response");

        // Third message: user with page content for turn-2
        EXPECT_EQ(messages[2].role, "user");
        ASSERT_EQ(messages[2].content.size(), 2u);
        VerifyPageTextBlock(FROM_HERE, messages[2].content[0],
                            "Content for second turn");
        VerifyTextBlock(FROM_HERE, messages[2].content[1],
                        "Second human message");

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
        run_loop.Quit();
      });

  PageContent page_content1("Content for first turn", false);
  PageContent page_content2("Content for second turn", false);

  std::vector<mojom::ConversationTurnPtr> history;

  // First turn pair
  auto turn1 = mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "First human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt, nullptr);
  history.push_back(std::move(turn1));

  auto response1 = mojom::ConversationTurn::New(
      "response-1", mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "First assistant response", std::nullopt,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt, nullptr);
  history.push_back(std::move(response1));

  // Second turn
  auto turn2 = mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Second human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt, nullptr);
  history.push_back(std::move(turn2));

  engine_->GenerateAssistantResponse(
      {{"turn-1", {page_content1}}, {"turn-2", {page_content2}}},
      std::move(history), false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&](EngineConsumer::GenerationResult) { /* handled above */ }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_MultiplePageContents_MultipleTurns_TooLong) {
  // Create page contents with specific lengths for truncation testing
  // Using lengths that will trigger truncation behavior similar to the OAI test
  PageContent page_content_1(std::string(35, '1'), false);
  PageContent page_content_2(std::string(35, '2'), false);
  PageContent page_content_3(std::string(35, '3'), false);

  // Create conversation history with multiple turns
  std::vector<mojom::ConversationTurnPtr> history;

  auto turn1 = mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Human message 1", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt, nullptr);
  history.push_back(std::move(turn1));

  auto turn2 = mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Human message 2", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt, nullptr);
  history.push_back(std::move(turn2));

  auto* mock_api_client = GetMockConversationAPIV2Client();

  auto test_content_truncation =
      [&](uint32_t max_length, std::vector<std::string> expected_contents) {
        SCOPED_TRACE(absl::StrFormat("Testing Truncation with max length: %d",
                                     max_length));
        engine_->SetMaxAssociatedContentLengthForTesting(max_length);

        base::RunLoop run_loop;
        EXPECT_CALL(*mock_api_client, PerformRequest)
            .WillOnce([&](std::vector<OAIMessage> messages,
                          std::optional<base::Value::List> oai_tool_definitions,
                          const std::optional<std::string>& preferred_tool_name,
                          mojom::ConversationCapability conversation_capability,
                          EngineConsumer::GenerationDataCallback data_callback,
                          EngineConsumer::GenerationCompletedCallback callback,
                          const std::optional<std::string>& model_name) {
              // Extract all text content from all messages for verification
              std::vector<std::string> actual_contents;
              for (const auto& msg : messages) {
                for (const auto& block : msg.content) {
                  if (block->is_text_content_block()) {
                    actual_contents.push_back(
                        block->get_text_content_block()->text);
                  } else if (block->is_page_text_content_block()) {
                    actual_contents.push_back(
                        block->get_page_text_content_block()->text);
                  } else {
                    FAIL() << "Unexpected block type"
                           << static_cast<int>(block->which());
                  }
                }
              }

              EXPECT_EQ(actual_contents.size(), expected_contents.size());
              for (size_t i = 0; i < expected_contents.size(); ++i) {
                SCOPED_TRACE(absl::StrFormat("Checking content %zu (max: %d)",
                                             i, max_length));
                if (i < actual_contents.size()) {
                  EXPECT_EQ(actual_contents[i], expected_contents[i]);
                }
              }

              auto completion_event =
                  mojom::ConversationEntryEvent::NewCompletionEvent(
                      mojom::CompletionEvent::New(""));
              std::move(callback).Run(
                  base::ok(EngineConsumer::GenerationResultData(
                      std::move(completion_event), std::nullopt)));
              run_loop.Quit();
            });

        engine_->GenerateAssistantResponse(
            {{"turn-1", {page_content_1, page_content_2}},
             {"turn-2", {page_content_3}}},
            history, false, {}, std::nullopt,
            mojom::ConversationCapability::CHAT, base::DoNothing(),
            base::DoNothing());
        run_loop.Run();
        testing::Mock::VerifyAndClearExpectations(mock_api_client);
      };

  // Test case: Max Length = 105 (should include all page contents)
  // Total content: 35 + 35 + 35 = 105 chars
  test_content_truncation(105, {
                                   std::string(35, '2'),
                                   std::string(35, '1'),
                                   "Human message 1",
                                   std::string(35, '3'),
                                   "Human message 2",
                               });

  // Test case: Max Length = 100
  // Content 3: 35 + Content 2: 35 + Content 1: 30 chars = 100 chars
  test_content_truncation(100, {
                                   std::string(35, '2'),
                                   std::string(30, '1'),
                                   "Human message 1",
                                   std::string(35, '3'),
                                   "Human message 2",
                               });

  // Test case: Max Length = 70
  // Content 3: 35 chars + Content 2: 35 chars = 70 chars
  test_content_truncation(70, {
                                  std::string(35, '2'),
                                  "Human message 1",
                                  std::string(35, '3'),
                                  "Human message 2",
                              });

  // Test case: Max Length = 65
  // Content 3: 35 + Content 2: 30 chars = 65 chars
  test_content_truncation(65, {
                                  std::string(30, '2'),
                                  "Human message 1",
                                  std::string(35, '3'),
                                  "Human message 2",
                              });

  // Test case: Max Length = 35 (should include only page content 3)
  test_content_truncation(35, {
                                  "Human message 1",
                                  std::string(35, '3'),
                                  "Human message 2",
                              });

  // Test case: Max Length = 10 (should include only partial content 3)
  test_content_truncation(10, {
                                  "Human message 1",
                                  std::string(10, '3'),
                                  "Human message 2",
                              });

  // Test case: Max Length = 0 (all page content omitted)
  test_content_truncation(0, {
                                 "Human message 1",
                                 "Human message 2",
                             });
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_UploadImage) {
  auto uploaded_images =
      CreateSampleUploadedFiles(3, mojom::UploadedFileType::kImage);
  auto screenshot_images =
      CreateSampleUploadedFiles(3, mojom::UploadedFileType::kScreenshot);
  uploaded_images.insert(uploaded_images.end(),
                         std::make_move_iterator(screenshot_images.begin()),
                         std::make_move_iterator(screenshot_images.end()));
  constexpr char kTestPrompt[] = "Tell the user what these images are?";
  constexpr char kAssistantResponse[] =
      "There are images of a lion, a dragon and a stag. And screenshots appear "
      "to be telling the story of Game of Thrones";

  // Build expected JSON format
  std::string expected_messages = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {"type": "text", "text": "These images are uploaded by the user"},
            {"type": "image_url", "image_url": {"url": "%s"}},
            {"type": "image_url", "image_url": {"url": "%s"}},
            {"type": "image_url", "image_url": {"url": "%s"}},
            {"type": "text", "text": "These images are screenshots"},
            {"type": "image_url", "image_url": {"url": "%s"}},
            {"type": "image_url", "image_url": {"url": "%s"}},
            {"type": "image_url", "image_url": {"url": "%s"}},
            {"type": "text", "text": "%s"}
          ]
        }
      ])",
      EngineConsumer::GetImageDataURL(uploaded_images[0]->data),
      EngineConsumer::GetImageDataURL(uploaded_images[1]->data),
      EngineConsumer::GetImageDataURL(uploaded_images[2]->data),
      EngineConsumer::GetImageDataURL(uploaded_images[3]->data),
      EngineConsumer::GetImageDataURL(uploaded_images[4]->data),
      EngineConsumer::GetImageDataURL(uploaded_images[5]->data), kTestPrompt);

  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 1 message
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");

        // Content blocks: images text + 3 images + screenshots text +
        // 3 screenshots + prompt = 9
        ASSERT_EQ(messages[0].content.size(), 9u);

        // Check uploaded images text
        VerifyTextBlock(FROM_HERE, messages[0].content[0],
                        "These images are uploaded by the user");

        // Check 3 uploaded images
        size_t image_idx = 0;
        for (size_t i = 1; i <= 3; ++i) {
          VerifyImageBlock(FROM_HERE, messages[0].content[i],
                           GURL(EngineConsumer::GetImageDataURL(
                               uploaded_images[image_idx++]->data)));
        }

        // Check screenshots text
        VerifyTextBlock(FROM_HERE, messages[0].content[4],
                        "These images are screenshots");

        // Check 3 screenshots
        for (size_t i = 5; i <= 7; ++i) {
          VerifyImageBlock(FROM_HERE, messages[0].content[i],
                           GURL(EngineConsumer::GetImageDataURL(
                               uploaded_images[image_idx++]->data)));
        }

        // Check prompt
        VerifyTextBlock(FROM_HERE, messages[0].content[8], kTestPrompt);

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(kAssistantResponse));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::UNSPECIFIED,
      "What are these images?", kTestPrompt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, Clone(uploaded_images),
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateAssistantResponse({}, history, false, {}, std::nullopt,
                                     mojom::ConversationCapability::CHAT,
                                     base::DoNothing(), future.GetCallback());
  EXPECT_EQ(future.Take(),
            EngineConsumer::GenerationResultData(
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New(kAssistantResponse)),
                std::nullopt /* model_key */));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithUploadedPdfFiles) {
  PageContent page_content("This is a page about The Mandalorian.", false);

  // Create test uploaded PDF files
  auto uploaded_files =
      CreateSampleUploadedFiles(2, mojom::UploadedFileType::kPdf);

  constexpr char kTestPrompt[] = "Can you analyze these PDFs?";

  // Build expected JSON format
  std::string expected_messages = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {"type": "brave-page-text", "text": "This is a page about The Mandalorian."},
            {"type": "text", "text": "These PDFs are uploaded by the user"},
            {"type": "file", "file": {"filename": "%s", "file_data": "%s"}},
            {"type": "file", "file": {"filename": "%s", "file_data": "%s"}},
            {"type": "text", "text": "%s"}
          ]
        }
      ])",
      uploaded_files[0]->filename,
      EngineConsumer::GetPdfDataURL(uploaded_files[0]->data),
      uploaded_files[1]->filename,
      EngineConsumer::GetPdfDataURL(uploaded_files[1]->data), kTestPrompt);

  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 1 message
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");

        // Content: page text + PDFs text + 2 PDFs + prompt = 5 blocks
        ASSERT_EQ(messages[0].content.size(), 5u);

        // Check page text
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            "This is a page about The Mandalorian.");

        // Check PDFs intro text
        VerifyTextBlock(FROM_HERE, messages[0].content[1],
                        "These PDFs are uploaded by the user");

        // Check first PDF
        VerifyFileBlock(
            FROM_HERE, messages[0].content[2],
            GURL(EngineConsumer::GetPdfDataURL(uploaded_files[0]->data)),
            uploaded_files[0]->filename);

        // Check second PDF
        VerifyFileBlock(
            FROM_HERE, messages[0].content[3],
            GURL(EngineConsumer::GetPdfDataURL(uploaded_files[1]->data)),
            uploaded_files[1]->filename);

        // Check final prompt
        VerifyTextBlock(FROM_HERE, messages[0].content[4], kTestPrompt);

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = kTestPrompt;
  turn->uploaded_files = Clone(uploaded_files);
  history.push_back(std::move(turn));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateAssistantResponse({{"turn-1", {page_content}}}, history,
                                     false, {}, std::nullopt,
                                     mojom::ConversationCapability::CHAT,
                                     base::DoNothing(), future.GetCallback());
  EXPECT_TRUE(future.Wait());
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithMixedUploadedFiles) {
  PageContent page_content("This is a page about The Mandalorian.", false);

  // Create test uploaded files of different types
  auto uploaded_files = std::vector<mojom::UploadedFilePtr>();

  // Add a PDF file
  auto pdf_file = mojom::UploadedFile::New();
  pdf_file->filename = "document.pdf";
  pdf_file->filesize = 1024;
  pdf_file->data = {0x25, 0x50, 0x44, 0x46};  // PDF magic bytes
  pdf_file->type = mojom::UploadedFileType::kPdf;
  uploaded_files.push_back(std::move(pdf_file));

  // Add an image file
  auto image_file = mojom::UploadedFile::New();
  image_file->filename = "image.jpg";
  image_file->filesize = 512;
  image_file->data = {0xFF, 0xD8, 0xFF};  // JPEG magic bytes
  image_file->type = mojom::UploadedFileType::kImage;
  uploaded_files.push_back(std::move(image_file));

  // Add a screenshot
  auto screenshot_file = mojom::UploadedFile::New();
  screenshot_file->filename = "screenshot.png";
  screenshot_file->filesize = 768;
  screenshot_file->data = {0x89, 0x50, 0x4E, 0x47};  // PNG magic bytes
  screenshot_file->type = mojom::UploadedFileType::kScreenshot;
  uploaded_files.push_back(std::move(screenshot_file));

  constexpr char kTestPrompt[] = "Can you analyze these files?";

  // Build expected JSON format
  std::string expected_messages = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {"type": "brave-page-text", "text": "This is a page about The Mandalorian."},
            {"type": "text", "text": "These images are uploaded by the user"},
            {"type": "image_url", "image_url": {"url": "%s"}},
            {"type": "text", "text": "These images are screenshots"},
            {"type": "image_url", "image_url": {"url": "%s"}},
            {"type": "text", "text": "These PDFs are uploaded by the user"},
            {"type": "file", "file": {"filename": "document.pdf", "file_data": "%s"}},
            {"type": "text", "text": "%s"}
          ]
        }
      ])",
      EngineConsumer::GetImageDataURL(uploaded_files[1]->data),
      EngineConsumer::GetImageDataURL(uploaded_files[2]->data),
      EngineConsumer::GetPdfDataURL(uploaded_files[0]->data), kTestPrompt);

  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 1 message
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");

        // Content: page + images_text + 1_image + screenshots_text +
        // 1_screenshot + pdfs_text + 1_pdf + prompt = 8 blocks
        ASSERT_EQ(messages[0].content.size(), 8u);

        size_t idx = 0;

        // Check page text
        VerifyPageTextBlock(FROM_HERE, messages[0].content[idx],
                            "This is a page about The Mandalorian.");
        idx++;

        // Check images intro text
        VerifyTextBlock(FROM_HERE, messages[0].content[idx],
                        "These images are uploaded by the user");
        idx++;

        // Check image (uploaded_files[1])
        VerifyImageBlock(
            FROM_HERE, messages[0].content[idx],
            GURL(EngineConsumer::GetImageDataURL(uploaded_files[1]->data)));
        idx++;

        // Check screenshots intro text
        VerifyTextBlock(FROM_HERE, messages[0].content[idx],
                        "These images are screenshots");
        idx++;

        // Check screenshot (uploaded_files[2])
        VerifyImageBlock(
            FROM_HERE, messages[0].content[idx],
            GURL(EngineConsumer::GetImageDataURL(uploaded_files[2]->data)));
        idx++;

        // Check PDFs intro text
        VerifyTextBlock(FROM_HERE, messages[0].content[idx],
                        "These PDFs are uploaded by the user");
        idx++;

        // Check PDF (uploaded_files[0])
        VerifyFileBlock(
            FROM_HERE, messages[0].content[idx],
            GURL(EngineConsumer::GetPdfDataURL(uploaded_files[0]->data)),
            "document.pdf");
        idx++;

        // Check final prompt
        VerifyTextBlock(FROM_HERE, messages[0].content[idx], kTestPrompt);

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = kTestPrompt;
  turn->uploaded_files = Clone(uploaded_files);
  history.push_back(std::move(turn));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateAssistantResponse({{"turn-1", {page_content}}}, history,
                                     false, {}, std::nullopt,
                                     mojom::ConversationCapability::CHAT,
                                     base::DoNothing(), future.GetCallback());
  EXPECT_TRUE(future.Wait());
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithOnlyPdfFiles) {
  // Test case with only PDF files, no page content
  auto uploaded_files =
      CreateSampleUploadedFiles(1, mojom::UploadedFileType::kPdf);

  constexpr char kTestPrompt[] = "What's in this PDF?";

  // Build expected JSON format
  std::string expected_messages = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {"type": "text", "text": "These PDFs are uploaded by the user"},
            {"type": "file", "file": {"filename": "%s", "file_data": "%s"}},
            {"type": "text", "text": "%s"}
          ]
        }
      ])",
      uploaded_files[0]->filename,
      EngineConsumer::GetPdfDataURL(uploaded_files[0]->data), kTestPrompt);

  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 1 message
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");

        // Content: PDFs text + 1 PDF + prompt = 3 blocks
        ASSERT_EQ(messages[0].content.size(), 3u);

        // Check PDFs intro text
        VerifyTextBlock(FROM_HERE, messages[0].content[0],
                        "These PDFs are uploaded by the user");

        // Check PDF
        VerifyFileBlock(
            FROM_HERE, messages[0].content[1],
            GURL(EngineConsumer::GetPdfDataURL(uploaded_files[0]->data)),
            uploaded_files[0]->filename);

        // Check final prompt
        VerifyTextBlock(FROM_HERE, messages[0].content[2], kTestPrompt);

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = kTestPrompt;
  turn->uploaded_files = Clone(uploaded_files);
  history.push_back(std::move(turn));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateAssistantResponse({}, history, false, {}, std::nullopt,
                                     mojom::ConversationCapability::CHAT,
                                     base::DoNothing(), future.GetCallback());
  EXPECT_TRUE(future.Wait());
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithMultiplePdfFiles) {
  constexpr char kTestPrompt[] = "Can you compare these three PDFs?";
  PageContent page_content("This is a page about The Mandalorian.", false);

  // Create multiple PDF files
  auto uploaded_files =
      CreateSampleUploadedFiles(3, mojom::UploadedFileType::kPdf);

  // Build expected JSON format
  std::string expected_messages = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {"type": "brave-page-text", "text": "This is a page about The Mandalorian."},
            {"type": "text", "text": "These PDFs are uploaded by the user"},
            {"type": "file", "file": {"filename": "%s", "file_data": "%s"}},
            {"type": "file", "file": {"filename": "%s", "file_data": "%s"}},
            {"type": "file", "file": {"filename": "%s", "file_data": "%s"}},
            {"type": "text", "text": "%s"}
          ]
        }
      ])",
      uploaded_files[0]->filename,
      EngineConsumer::GetPdfDataURL(uploaded_files[0]->data),
      uploaded_files[1]->filename,
      EngineConsumer::GetPdfDataURL(uploaded_files[1]->data),
      uploaded_files[2]->filename,
      EngineConsumer::GetPdfDataURL(uploaded_files[2]->data), kTestPrompt);

  auto* mock_api_client = GetMockConversationAPIV2Client();
  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Verify we have one message
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");

        // Verify content blocks:
        // page_text + PDFs_intro_text + 3_PDF_blocks + prompt = 6 blocks
        ASSERT_EQ(messages[0].content.size(), 6u);

        // Check page text content block
        VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                            "This is a page about The Mandalorian.");

        // Check PDF intro text
        VerifyTextBlock(FROM_HERE, messages[0].content[1],
                        "These PDFs are uploaded by the user");

        // Check the 3 PDF file content blocks
        size_t file_idx = 0;
        for (size_t i = 2; i <= 4; ++i) {
          auto& uploaded_file = uploaded_files[file_idx++];
          VerifyFileBlock(
              FROM_HERE, messages[0].content[i],
              GURL(EngineConsumer::GetPdfDataURL(uploaded_file->data)),
              uploaded_file->filename);
        }

        // Check the final prompt text content block
        VerifyTextBlock(FROM_HERE, messages[0].content[5], kTestPrompt);

        // Verify JSON serialization matches expected format
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->uuid = "turn-1";
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = kTestPrompt;
  turn->uploaded_files = Clone(uploaded_files);
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse({{"turn-1", {page_content}}}, history,
                                     false, {}, std::nullopt,
                                     mojom::ConversationCapability::CHAT,
                                     base::DoNothing(), future.GetCallback());
  EXPECT_TRUE(future.Wait());
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_WithSkill) {
  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  // Create conversation history with skill entry
  EngineConsumer::ConversationHistory conversation_history;
  auto skill_entry =
      mojom::SkillEntry::New("summarize", "Please summarize the content");
  conversation_history.push_back(mojom::ConversationTurn::New(
      "uuid", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "/summarize What is artificial intelligence?", std::nullopt /* prompt */,
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_files */, std::move(skill_entry), false,
      std::nullopt /* model_key */, nullptr /* near_verification_status */));

  MockConversationAPIV2Client* mock_client = GetMockConversationAPIV2Client();

  // Expect that PerformRequest is called with a message containing both
  // skill definition and main user query as content blocks
  EXPECT_CALL(*mock_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 1 message with 2 content blocks: skill
        // definition + query
        ASSERT_EQ(messages.size(), 1u);

        // Should have 1 user message
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 2u);

        // First content block should be the skill definition
        VerifyTextBlock(FROM_HERE, messages[0].content[0],
                        "When handling the request, interpret '/summarize' as "
                        "'Please summarize the content'");

        // Second content block should be the actual user message
        VerifyTextBlock(FROM_HERE, messages[0].content[1],
                        "/summarize What is artificial intelligence?");

        // Mock successful response
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("AI is a technology..."));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, conversation_history, false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT,
      base::BindRepeating([](EngineConsumer::GenerationResultData) {}),
      future.GetCallback());

  // Wait for the response
  auto result = future.Take();
  EXPECT_TRUE(result.has_value());
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_ToolUse) {
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "What is the weather in Santa Barbara?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  std::vector<mojom::ContentBlockPtr> tool_output_content_blocks;
  tool_output_content_blocks.push_back(mojom::ContentBlock::NewTextContentBlock(
      mojom::TextContentBlock::New("{ \"temperature\":\"75F\" }")));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("First I'll look up the weather...")));
  response_events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_123", "{\"location\":\"Santa Barbara\"}",
          std::move(tool_output_content_blocks), nullptr, false)));

  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "First I'll look up the weather...", std::nullopt /* prompt */,
      std::nullopt, std::move(response_events), base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  std::string expected_messages = R"([
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What is the weather in Santa Barbara?"
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "First I'll look up the weather..."
        }
      ],
      "tool_calls": [
        {
          "id": "call_123",
          "type": "function",
          "function": {
            "name": "get_weather",
            "arguments": "{\"location\":\"Santa Barbara\"}"
          }
        }
      ]
    },
    {
      "role": "tool",
      "tool_call_id": "call_123",
      "content": [
        {
          "type": "text",
          "text": "{ \"temperature\":\"75F\" }"
        }
      ]
    }
  ])";
  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // One user turn, one assistant turn, one tool turn
        EXPECT_EQ(messages.size(), 3u);
        EXPECT_THAT(
            mock_api_client->GetMessagesJson(std::move(messages)),
            base::test::IsJson(base::test::ParseJson(expected_messages)));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, history, false, {}, std::nullopt, mojom::ConversationCapability::CHAT,
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_MultipleToolUse) {
  // Responses can contain multiple tool use events
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "What is the weather in Santa Barbara?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("First I'll look up the weather...")));

  std::vector<mojom::ContentBlockPtr> temperature_tool_output_content_blocks;
  temperature_tool_output_content_blocks.push_back(
      mojom::ContentBlock::NewTextContentBlock(
          mojom::TextContentBlock::New("{ \"temperature\":\"75F\" }")));
  response_events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_temperature", "call_123", "{\"location\":\"Santa Barbara\"}",
          std::move(temperature_tool_output_content_blocks), nullptr, false)));

  std::vector<mojom::ContentBlockPtr> wind_tool_output_content_blocks;
  wind_tool_output_content_blocks.push_back(
      mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
          "{ \"speed\":\"25mph\", \"direction\":\"NW\" }")));
  response_events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_wind", "call_1234", "{\"location\":\"Santa Barbara\"}",
          std::move(wind_tool_output_content_blocks), nullptr, false)));

  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "First I'll look up the weather...", std::nullopt /* prompt */,
      std::nullopt, std::move(response_events), base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  std::string expected_messages = R"([
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What is the weather in Santa Barbara?"
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "First I'll look up the weather..."
        }
      ],
      "tool_calls": [
        {
          "id": "call_123",
          "type": "function",
          "function": {
            "name": "get_temperature",
            "arguments": "{\"location\":\"Santa Barbara\"}"
          }
        },
        {
          "id": "call_1234",
          "type": "function",
          "function": {
            "name": "get_wind",
            "arguments": "{\"location\":\"Santa Barbara\"}"
          }
        }
      ]
    },
    {
      "role": "tool",
      "tool_call_id": "call_123",
      "content": [
        {
          "type": "text",
          "text": "{ \"temperature\":\"75F\" }"
        }
      ]
    },
    {
      "role": "tool",
      "tool_call_id": "call_1234",
      "content": [
        {
          "type": "text",
          "text": "{ \"speed\":\"25mph\", \"direction\":\"NW\" }"
        }
      ]
    }
  ])";
  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // One user turn, one assistant turn, two tool turns
        EXPECT_EQ(messages.size(), 4u);
        EXPECT_THAT(base::test::ParseJson(
                        mock_api_client->GetMessagesJson(std::move(messages))),
                    base::test::IsJson(expected_messages));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, history, false, {}, std::nullopt, mojom::ConversationCapability::CHAT,
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_MultipleToolUseWithLargeContent) {
  EngineConsumer::ConversationHistory history;

  // Generate 3 tool use requests and the first one should be removed
  // since kMaxCountLargeToolUseEvents is 2.
  // Content considered as "large" is any image, or text if its size is > 1000,
  // so we'll include both those types.
  // This test also covers multiple tool use events and different content types,
  // ensuring the order of calls is preserved as well as accompanying completion
  // text.
  std::string large_text_content(1500, 'a');
  std::string image_url = "data:image/png;base64,ABC=";
  for (int i = 0; i < 3; ++i) {
    history.push_back(mojom::ConversationTurn::New(
        "turn-" + base::NumberToString(i * 3), mojom::CharacterType::HUMAN,
        mojom::ActionType::QUERY, "What is this web page about?",
        std::nullopt /* prompt */, std::nullopt, std::nullopt,
        base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */,
        false, std::nullopt /* model_key */,
        nullptr /* near_verification_status */));
    std::vector<mojom::ContentBlockPtr> tool_output_content_blocks;
    if (i == 0 || i == 2) {
      tool_output_content_blocks.push_back(
          mojom::ContentBlock::NewImageContentBlock(
              mojom::ImageContentBlock::New(GURL(image_url))));
    } else {
      tool_output_content_blocks.push_back(
          mojom::ContentBlock::NewTextContentBlock(
              mojom::TextContentBlock::New(large_text_content)));
    }
    std::vector<mojom::ConversationEntryEventPtr> response_events;
    response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New("First I'll look up the page...")));
    response_events.push_back(
        mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
            "get_page_content", base::StrCat({"call_123", base::ToString(i)}),
            "{}", std::move(tool_output_content_blocks), nullptr, false)));
    history.push_back(mojom::ConversationTurn::New(
        "turn-" + base::NumberToString(i * 3 + 1),
        mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        "First I'll look up the page...", std::nullopt /* prompt */,
        std::nullopt, std::move(response_events), base::Time::Now(),
        std::nullopt, std::nullopt, nullptr /* skill */, false,
        std::nullopt /* model_key */, nullptr /* near_verification_status */));
    history.push_back(mojom::ConversationTurn::New(
        "turn-" + base::NumberToString(i * 3 + 2),
        mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        "The page has some great content", std::nullopt /* prompt */,
        std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
        std::nullopt, nullptr /* skill */, false, std::nullopt /* model_key */,
        nullptr /* near_verification_status */));
  }

  std::string expected_messages = R"([
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What is this web page about?"
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "First I'll look up the page..."
        }
      ],
      "tool_calls": [
        {
          "id": "call_1230",
          "type": "function",
          "function": {
            "name": "get_page_content",
            "arguments": "{}"
          }
        }
      ]
    },
    {
      "role": "tool",
      "tool_call_id": "call_1230",
      "content": [
        {
          "type": "text",
          "text": "[Large result removed to save space for subsequent results]"
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "The page has some great content"
        }
      ]
    },
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What is this web page about?"
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "First I'll look up the page..."
        }
      ],
      "tool_calls": [
        {
          "id": "call_1231",
          "type": "function",
          "function": {
            "name": "get_page_content",
            "arguments": "{}"
          }
        }
      ]
    },
    {
      "role": "tool",
      "tool_call_id": "call_1231",
      "content": [
        {
          "type": "text",
          "text": ")" + large_text_content +
                                  R"("
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "The page has some great content"
        }
      ]
    },
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What is this web page about?"
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "First I'll look up the page..."
        }
      ],
      "tool_calls": [
        {
          "id": "call_1232",
          "type": "function",
          "function": {
            "name": "get_page_content",
            "arguments": "{}"
          }
        }
      ]
    },
    {
      "role": "tool",
      "tool_call_id": "call_1232",
      "content": [
        {
          "type": "image_url",
          "image_url": {
            "url": "data:image/png;base64,ABC="
          }
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "The page has some great content"
        }
      ]
    }
  ])";
  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_THAT(base::test::ParseJson(
                        mock_api_client->GetMessagesJson(std::move(messages))),
                    base::test::IsJson(expected_messages));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, history, false, {}, std::nullopt, mojom::ConversationCapability::CHAT,
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateAssistantResponse_ToolUseNoOutput) {
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "What is the weather in Santa Barbara?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("First I'll look up the weather...")));
  response_events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_123", "{\"location\":\"Santa Barbara\"}",
          std::nullopt, nullptr, false)));

  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "First I'll look up the weather...", std::nullopt /* prompt */,
      std::nullopt, std::move(response_events), base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  // If somehow the conversation is sent without the tool output, the
  // request should not include the tool request, since most LLM APIs will fail
  // in that scenario. This should be prevented by the callers.
  std::string expected_messages = R"([
    {
      "role": "user",
      "content": [
        {
          "type": "text",
          "text": "What is the weather in Santa Barbara?"
        }
      ]
    },
    {
      "role": "assistant",
      "content": [
        {
          "type": "text",
          "text": "First I'll look up the weather..."
        }
      ]
    }
  ])";
  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 2u);
        EXPECT_THAT(
            mock_api_client->GetMessagesJson(std::move(messages)),
            base::test::IsJson(base::test::ParseJson(expected_messages)));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, history, false, {}, std::nullopt, mojom::ConversationCapability::CHAT,
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest, GenerateQuestionSuggestions) {
  PageContent page_content("Sample page content.", false);
  PageContent video_content("Sample video content.", true);
  PageContents page_contents{page_content, video_content};

  std::string expected_messages = R"([
    {
      "role": "user",
      "content": [
        {"type": "brave-video-transcript", "text": "Sample video content."},
        {"type": "brave-page-text", "text": "Sample page content."},
        {"type": "brave-request-questions", "text": ""}
      ]
    }
  ])";

  auto* mock_api_client = GetMockConversationAPIV2Client();

  // Test successful response
  {
    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          ASSERT_EQ(messages.size(), 1u);
          EXPECT_EQ(messages[0].role, "user");
          ASSERT_EQ(messages[0].content.size(), 3u);

          // First content block should be video transcript
          VerifyVideoTranscriptBlock(FROM_HERE, messages[0].content[0],
                                     "Sample video content.");

          // Second content block should be page text
          VerifyPageTextBlock(FROM_HERE, messages[0].content[1],
                              "Sample page content.");

          // Third content block should be request questions
          VerifySimpleRequestBlock(FROM_HERE, messages[0].content[2],
                                   mojom::SimpleRequestType::kRequestQuestions);

          // Verify JSON serialization matches expected format
          EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                    FormatComparableMessagesJson(expected_messages));

          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("question1|question2|question3"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    engine_->GenerateQuestionSuggestions(
        page_contents,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_TRUE(result.has_value());
          std::vector<std::string> expected_questions = {
              "question1", "question2", "question3"};
          EXPECT_EQ(*result, expected_questions);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test error response
  {
    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          std::move(callback).Run(
              base::unexpected(mojom::APIError::RateLimitReached));
        });

    engine_->GenerateQuestionSuggestions(
        page_contents,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_FALSE(result.has_value());
          EXPECT_EQ(result.error(), mojom::APIError::RateLimitReached);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test empty completion event
  {
    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New(""));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    engine_->GenerateQuestionSuggestions(
        page_contents,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_FALSE(result.has_value());
          EXPECT_EQ(result.error(), mojom::APIError::InternalError);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test null event
  {
    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<OAIMessage> messages,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          std::move(callback).Run(base::ok(
              EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
        });

    engine_->GenerateQuestionSuggestions(
        page_contents,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_FALSE(result.has_value());
          EXPECT_EQ(result.error(), mojom::APIError::InternalError);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateRewriteSuggestion_UnsupportedActionTypeReturnsInternalError) {
  auto* client = GetMockConversationAPIV2Client();

  // Expect PerformRequest is NOT called for unsupported action types
  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _, _, _)).Times(0);

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateRewriteSuggestion("Hello World",
                                     mojom::ActionType::CREATE_TAGLINE,
                                     base::DoNothing(), future.GetCallback());

  auto result = future.Take();
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);

  testing::Mock::VerifyAndClearExpectations(client);
}

class EngineConsumerConversationAPIV2UnitTest_GenerateRewrite
    : public EngineConsumerConversationAPIV2UnitTest,
      public testing::WithParamInterface<GenerateRewriteTestParam> {};

TEST_P(EngineConsumerConversationAPIV2UnitTest_GenerateRewrite,
       GenerateRewriteSuggestion) {
  GenerateRewriteTestParam params = GetParam();
  auto* client = GetMockConversationAPIV2Client();
  base::RunLoop run_loop;

  std::string test_text = "Hello World";
  std::string expected_response = "Rewritten text here.";

  // Build expected JSON format
  std::string expected_messages;
  if (params.expected_content_type ==
      mojom::ContentBlock::Tag::kChangeToneContentBlock) {
    expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {"type": "brave-page-excerpt", "text": "%s"},
              {"type": "%s", "text": "", "tone": "%s"}
            ]
          }
        ])",
        test_text, params.expected_type_string, params.expected_payload);
  } else {
    expected_messages = absl::StrFormat(
        R"([
          {
            "role": "user",
            "content": [
              {"type": "brave-page-excerpt", "text": "%s"},
              {"type": "%s", "text": "%s"}
            ]
          }
        ])",
        test_text, params.expected_type_string, params.expected_payload);
  }

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _, _, _))
      .WillOnce(
          [&](std::vector<OAIMessage> messages,
              std::optional<base::Value::List> oai_tool_definitions,
              const std::optional<std::string>& preferred_tool_name,
              mojom::ConversationCapability conversation_capability,
              EngineConsumer::GenerationDataCallback data_callback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::string>& model_name) {
            // Verify conversation capability is CHAT
            EXPECT_EQ(conversation_capability,
                      mojom::ConversationCapability::CHAT);

            // Verify no tool definitions for rewrite requests
            EXPECT_FALSE(oai_tool_definitions.has_value());
            EXPECT_FALSE(preferred_tool_name.has_value());

            // Verify messages structure
            ASSERT_GE(messages.size(), 1u);

            // First message should contain the text and action content block
            const auto& first_message = messages[0];
            EXPECT_EQ(first_message.role, "user");
            ASSERT_GE(first_message.content.size(), 2u);

            // First content block should be the page excerpt
            VerifyPageExcerptBlock(FROM_HERE, first_message.content[0],
                                   test_text);

            // Second content block should be the action type
            ASSERT_EQ(first_message.content[1]->which(),
                      params.expected_content_type);

            // Verify the content data, should have tone for change tone type,
            // empty text otherwise.
            if (params.expected_content_type ==
                mojom::ContentBlock::Tag::kChangeToneContentBlock) {
              VerifyChangeToneBlock(FROM_HERE, first_message.content[1], "",
                                    params.expected_payload);
            } else if (params.expected_content_type ==
                       mojom::ContentBlock::Tag::kSimpleRequestContentBlock) {
              ASSERT_TRUE(params.expected_simple_request_type.has_value());
              VerifySimpleRequestBlock(FROM_HERE, first_message.content[1],
                                       *params.expected_simple_request_type);
            } else {
              FAIL() << "Unexpected type: "
                     << static_cast<int>(params.expected_content_type);
            }

            // Verify JSON serialization matches expected format
            EXPECT_EQ(client->GetMessagesJson(std::move(messages)),
                      FormatComparableMessagesJson(expected_messages));

            // Return completion
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(expected_response)),
                    std::nullopt)));
          });

  engine_->GenerateRewriteSuggestion(
      test_text, params.action_type, base::DoNothing(),
      base::BindLambdaForTesting([&run_loop, &expected_response](
                                     EngineConsumer::GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->event);
        ASSERT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion,
                  expected_response);
        run_loop.Quit();
      }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    EngineConsumerConversationAPIV2UnitTest_GenerateRewrite,
    testing::Values(
        GenerateRewriteTestParam{
            "Paraphrase", mojom::ActionType::PARAPHRASE,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock, "",
            "brave-request-paraphrase", mojom::SimpleRequestType::kParaphrase},
        GenerateRewriteTestParam{
            "Improve", mojom::ActionType::IMPROVE,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock, "",
            "brave-request-improve-excerpt-language",
            mojom::SimpleRequestType::kImprove},
        GenerateRewriteTestParam{
            "Shorten", mojom::ActionType::SHORTEN,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock, "",
            "brave-request-shorten", mojom::SimpleRequestType::kShorten},
        GenerateRewriteTestParam{
            "Expand", mojom::ActionType::EXPAND,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock, "",
            "brave-request-expansion", mojom::SimpleRequestType::kExpand},
        GenerateRewriteTestParam{
            "Academic", mojom::ActionType::ACADEMICIZE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "academic",
            "brave-request-change-tone"},
        GenerateRewriteTestParam{
            "Professional", mojom::ActionType::PROFESSIONALIZE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "professional",
            "brave-request-change-tone"},
        GenerateRewriteTestParam{
            "Casual", mojom::ActionType::CASUALIZE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "casual",
            "brave-request-change-tone"},
        GenerateRewriteTestParam{
            "Funny", mojom::ActionType::FUNNY_TONE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "funny",
            "brave-request-change-tone"},
        GenerateRewriteTestParam{
            "Persuasive", mojom::ActionType::PERSUASIVE_TONE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock, "persuasive",
            "brave-request-change-tone"}),
    [](const testing::TestParamInfo<GenerateRewriteTestParam>& info) {
      return info.param.name;
    });

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateConversationTitle_Success) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  auto history = CreateSampleChatHistory(1);
  PageContentsMap page_contents;

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([](std::vector<OAIMessage> messages,
                   std::optional<base::Value::List>,
                   const std::optional<std::string>&,
                   mojom::ConversationCapability,
                   EngineConsumer::GenerationDataCallback,
                   EngineConsumer::GenerationCompletedCallback callback,
                   const std::optional<std::string>&) {
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("Understanding AI Basics")),
            std::nullopt)));
      });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_conversation_title_event());
  EXPECT_EQ(result->event->get_conversation_title_event()->title,
            "Understanding AI Basics");

  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateConversationTitle_InvalidHistory) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  PageContentsMap page_contents;

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "Hello",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr, false, std::nullopt, nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*mock_api_client, PerformRequest).Times(0);

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateConversationTitle_NetworkError) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  auto history = CreateSampleChatHistory(1);
  PageContentsMap page_contents;

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([](std::vector<OAIMessage>, std::optional<base::Value::List>,
                   const std::optional<std::string>&,
                   mojom::ConversationCapability,
                   EngineConsumer::GenerationDataCallback,
                   EngineConsumer::GenerationCompletedCallback callback,
                   const std::optional<std::string>&) {
        std::move(callback).Run(
            base::unexpected(mojom::APIError::RateLimitReached));
      });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateConversationTitle_EmptyTitle) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  auto history = CreateSampleChatHistory(1);
  PageContentsMap page_contents;

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([](std::vector<OAIMessage>, std::optional<base::Value::List>,
                   const std::optional<std::string>&,
                   mojom::ConversationCapability,
                   EngineConsumer::GenerationDataCallback,
                   EngineConsumer::GenerationCompletedCallback callback,
                   const std::optional<std::string>&) {
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("")),
            std::nullopt)));
      });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GenerateConversationTitle_TitleTooLong) {
  auto* mock_api_client = GetMockConversationAPIV2Client();
  auto history = CreateSampleChatHistory(1);
  PageContentsMap page_contents;

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([](std::vector<OAIMessage>, std::optional<base::Value::List>,
                   const std::optional<std::string>&,
                   mojom::ConversationCapability,
                   EngineConsumer::GenerationDataCallback,
                   EngineConsumer::GenerationCompletedCallback callback,
                   const std::optional<std::string>&) {
        std::string long_title(kMaxTitleLength + 1, 'x');
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(long_title)),
            std::nullopt)));
      });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest, GetSuggestedTopics) {
  auto [tabs, tabs_json_strings] =
      GetMockTabsAndExpectedTabsJsonString(2 * kTabListChunkSize, true);
  ASSERT_EQ(tabs.size(), 2 * kTabListChunkSize);
  ASSERT_EQ(tabs_json_strings.size(), 2u);

  std::string expected_messages1 = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {
              "type": "brave-suggest-focus-topics",
              "text": "%s"
            }
          ]
        }
      ])",
      tabs_json_strings[0]);
  std::string expected_messages2 = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {
              "type": "brave-suggest-focus-topics",
              "text": "%s"
            }
          ]
        }
      ])",
      tabs_json_strings[1]);
  std::string expected_messages3 = R"([
    {
      "role": "user",
      "content": [
        {
          "type": "brave-reduce-focus-topics",
          "text": "[\"topic1\",\"topic2\",\"topic3\",\"topic7\",\"topic3\",\"topic4\",\"topic5\",\"topic6\"]"
        }
      ]
    }
  ])";

  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(3)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages2));
        // Test response with newlines in the array
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\n  \"topic3\",\n  \"topic4\",\n  "
                    "\"topic5\",\n  \"topic6\"\n] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages3));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic3\", \"topic4\", "
                    "\"topic5\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GetSuggestedTopics(
      tabs,
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result,
                  std::vector<std::string>(
                      {"topic1", "topic3", "topic4", "topic5", "topic7"}));
      }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);

  // Any server error during getting suggested topics or get dudupe topics
  // would fail the request.
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(2)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages2));
        std::move(callback).Run(
            base::unexpected(mojom::APIError::RateLimitReached));
      });
  engine_->GetSuggestedTopics(
      tabs, base::BindLambdaForTesting(
                [&](base::expected<std::vector<std::string>, mojom::APIError>
                        result) {
                  ASSERT_FALSE(result.has_value());
                  EXPECT_EQ(result.error(), mojom::APIError::RateLimitReached);
                }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(3)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic3\", \"topic4\", \"topic5\", "
                    "\"topic6\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages3));
        std::move(callback).Run(
            base::unexpected(mojom::APIError::RateLimitReached));
      });
  engine_->GetSuggestedTopics(
      tabs, base::BindLambdaForTesting(
                [&](base::expected<std::vector<std::string>, mojom::APIError>
                        result) {
                  ASSERT_FALSE(result.has_value());
                  EXPECT_EQ(result.error(), mojom::APIError::RateLimitReached);
                }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);

  // GetSuggestedTopics response with unexpected structure would be skipped.
  std::string expected_messages3_skipped_invalid_response = R"([
    {
      "role": "user",
      "content": [
        {
          "type": "brave-reduce-focus-topics",
          "text": "[\"topic1\",\"topic2\",\"topic3\",\"topic7\"]"
        }
      ]
    }
  ])";
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(3)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("not well structured"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(
                      expected_messages3_skipped_invalid_response));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });
  engine_->GetSuggestedTopics(
      tabs,
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, std::vector<std::string>(
                               {"topic1", "topic2", "topic3", "topic7"}));
      }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);

  // Test dedupe response is not well structured.
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(3)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic3\", \"topic4\", \"topic5\", "
                    "\"topic6\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages3));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": \"not an array of strings\" }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });
  engine_->GetSuggestedTopics(
      tabs, base::BindLambdaForTesting(
                [&](base::expected<std::vector<std::string>, mojom::APIError>
                        result) {
                  ASSERT_FALSE(result.has_value());
                  EXPECT_EQ(result.error(), mojom::APIError::InternalError);
                }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);

  // Test calling DedupeTopics with empty topics.
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(2)
      .WillRepeatedly([&](std::vector<OAIMessage> messages,
                          std::optional<base::Value::List> oai_tool_definitions,
                          const std::optional<std::string>& preferred_tool_name,
                          mojom::ConversationCapability conversation_capability,
                          EngineConsumer::GenerationDataCallback data_callback,
                          EngineConsumer::GenerationCompletedCallback callback,
                          const std::optional<std::string>& model_name) {
        ASSERT_EQ(messages.size(), 1u);
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 1u);
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("\"topics\": []"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });
  engine_->GetSuggestedTopics(
      tabs, base::BindLambdaForTesting(
                [&](base::expected<std::vector<std::string>, mojom::APIError>
                        result) {
                  EXPECT_FALSE(result.has_value());
                  EXPECT_EQ(result.error(), mojom::APIError::InternalError);
                }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       GetSuggestedTopics_SingleTabChunk) {
  auto [tabs, tabs_json_strings] =
      GetMockTabsAndExpectedTabsJsonString(1, true);
  ASSERT_EQ(tabs.size(), 1u);
  ASSERT_EQ(tabs_json_strings.size(), 1u);

  std::string expected_messages = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {
              "type": "brave-suggest-focus-topics-emoji",
              "text": "%s"
            }
          ]
        }
      ])",
      tabs_json_strings[0]);

  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages));
        // Test response with newlines in the array
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\n  \"topic1\",\n  \"topic2\"\n] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GetSuggestedTopics(
      tabs,
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, std::vector<std::string>({"topic1", "topic2"}));
      }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest, GetFocusTabs) {
  // Get two full chunks of tabs for testing.
  auto [tabs, tabs_json_strings] =
      GetMockTabsAndExpectedTabsJsonString(2 * kTabListChunkSize, true);
  ASSERT_EQ(tabs.size(), 2 * kTabListChunkSize);
  ASSERT_EQ(tabs_json_strings.size(), 2u);

  std::string expected_messages1 = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {
              "type": "brave-filter-tabs",
              "text": "%s",
              "topic": "test_topic"
            }
          ]
        }
      ])",
      tabs_json_strings[0]);
  std::string expected_messages2 = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {
              "type": "brave-filter-tabs",
              "text": "%s",
              "topic": "test_topic"
            }
          ]
        }
      ])",
      tabs_json_strings[1]);

  auto* mock_api_client = GetMockConversationAPIV2Client();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(2)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id1\", \"id2\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages2));
        // Test response with newlines in the array
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\n  \"id75\",\n  \"id76\"\n] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GetFocusTabs(
      tabs, "test_topic",
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result,
                  std::vector<std::string>({"id1", "id2", "id75", "id76"}));
      }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);

  // Test 1 full chunk of tabs and 1 partial chunk of tabs.
  auto [tabs2, tabs_json_strings2] =
      GetMockTabsAndExpectedTabsJsonString(kTabListChunkSize + 5, true);
  ASSERT_EQ(tabs2.size(), kTabListChunkSize + 5);
  ASSERT_EQ(tabs_json_strings2.size(), 2u);

  expected_messages1 = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {
              "type": "brave-filter-tabs",
              "text": "%s",
              "topic": "test_topic2"
            }
          ]
        }
      ])",
      tabs_json_strings2[0]);
  expected_messages2 = absl::StrFormat(
      R"([
        {
          "role": "user",
          "content": [
            {
              "type": "brave-filter-tabs",
              "text": "%s",
              "topic": "test_topic2"
            }
          ]
        }
      ])",
      tabs_json_strings2[1]);

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(2)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id3\", \"id5\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(messages.size(), 1u);
        EXPECT_EQ(mock_api_client->GetMessagesJson(std::move(messages)),
                  FormatComparableMessagesJson(expected_messages2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id75\", \"id76\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GetFocusTabs(
      tabs2, "test_topic2",
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result,
                  std::vector<std::string>({"id3", "id5", "id75", "id76"}));
      }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);

  // Any server error would fail the request.
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(2)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id3\", \"id5\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        std::move(callback).Run(
            base::unexpected(mojom::APIError::RateLimitReached));
      });

  engine_->GetFocusTabs(
      tabs2, "test_topic2",
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), mojom::APIError::RateLimitReached);
      }));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);

  // Entry with unexpected structure would be skipped.
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(2)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id3\", \"id5\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "I don't follow human instructions."));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GetFocusTabs(
      tabs2, "test_topic2",
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, std::vector<std::string>({"id3", "id5"}));
      }));

  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       ModelNameNotOverriddenWithToolCalls) {
  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EngineConsumer::ConversationHistory conversation_history;

  conversation_history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "What is the weather?", std::nullopt /* prompt */, std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */));

  std::vector<mojom::ContentBlockPtr> tool_output_content_blocks;
  tool_output_content_blocks.push_back(mojom::ContentBlock::NewTextContentBlock(
      mojom::TextContentBlock::New("{ \"temperature\":\"75F\" }")));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("Let me check the weather...")));
  response_events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_123", "{\"location\":\"Santa Barbara\"}",
          std::move(tool_output_content_blocks), nullptr, false)));

  conversation_history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Let me check the weather...", std::nullopt /* prompt */, std::nullopt,
      std::move(response_events), base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, kClaudeSonnetModelKey,
      nullptr /* near_verification_status */));

  MockConversationAPIV2Client* mock_client = GetMockConversationAPIV2Client();

  EXPECT_CALL(*mock_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_FALSE(model_name.has_value());

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("The temperature is 75F"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, conversation_history, false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT,
      base::BindRepeating([](EngineConsumer::GenerationResultData) {}),
      future.GetCallback());

  auto result = future.Take();
  EXPECT_TRUE(result.has_value());
}

TEST_F(EngineConsumerConversationAPIV2UnitTest,
       RegenerateAnswerModelNameOverridden) {
  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EngineConsumer::ConversationHistory conversation_history;

  conversation_history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Tell me a joke", std::nullopt /* prompt */, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      kClaudeSonnetModelKey /* model_key */,
      nullptr /* near_verification_status */));

  MockConversationAPIV2Client* mock_client = GetMockConversationAPIV2Client();

  EXPECT_CALL(*mock_client, PerformRequest)
      .WillOnce([&](std::vector<OAIMessage> messages,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        ASSERT_TRUE(model_name.has_value());
        EXPECT_EQ(*model_name, kClaudeSonnetModelName);

        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "Why did the chicken cross the road?"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, conversation_history, false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT,
      base::BindRepeating([](EngineConsumer::GenerationResultData) {}),
      future.GetCallback());

  auto result = future.Take();
  EXPECT_TRUE(result.has_value());
}

}  // namespace ai_chat

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
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_v2_client.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/extended_content_block.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
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
  ExtendedContentBlockType expected_content_type;
  std::string expected_payload;
  std::string expected_type_string;
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
               const std::string& selected_language,
               std::optional<base::Value::List> oai_tool_definitions,
               const std::optional<std::string>& preferred_tool_name,
               mojom::ConversationCapability conversation_capability,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback,
               const std::optional<std::string>& model_name),
              (override));

  std::string GetMessagesJson(std::vector<OAIMessage> messages) {
    auto body = CreateJSONRequestBody(
        std::move(messages), "", std::nullopt, std::nullopt,
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

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_BasicMessage) {
  // Although these tests should likely only be testing the
  // EngineConsumerConversationAPI class, we also include testing some
  // functionality of the very related ConversationAPIClient class. Whilst
  // EngineConsumerConversationAPI merely converts from AI Chat schemas
  // such as mojom::ConversationTurn, to the Conversation API's
  // ConversationEvent, the ConversationAPIClient class also converts from
  // ConversationEvent to JSON. It's convenient to test both here but more
  // exhaustive tests of  ConversationAPIClient are performed in its own
  // unit test suite.
  PageContent page_content(
      std::string(kTestingMaxAssociatedContentLength + 1, 'a'), false);
  std::string expected_page_content(kTestingMaxAssociatedContentLength, 'a');
  std::string expected_user_message_content =
      "Tell the user which show is this about?";
  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": ")" +
                                expected_page_content + R"("},
    {"role": "user", "type": "chatMessage", "content": ")" +
                                expected_user_message_content + R"("}
  ])";
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Some structured EXPECT calls to catch nicer errors first
        EXPECT_EQ(conversation.size(), 2u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        // Page content should be truncated
        EXPECT_EQ(GetContentStrings(conversation[0].content)[0],
                  expected_page_content);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        // Match entire structure
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events));
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
      {{{"turn-1", {page_content}}}}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateEvents_BasicMessage_MultiAssociatedTruncates) {
  size_t content_length = kTestingMaxAssociatedContentLength / 2 + 10;
  PageContent page_content_1(std::string(content_length, 'a'), false);
  PageContent page_content_2(std::string(content_length, 'b'), false);
  // First content should be truncated to the remaining available space (as we
  // truncate the oldest page content first).
  std::string expected_page_content_1(
      kTestingMaxAssociatedContentLength - content_length, 'a');
  std::string expected_page_content_2(content_length, 'b');

  std::string expected_user_message_content =
      "Tell the user which show is this about?";
  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": ")" +
                                expected_page_content_2 + R"("},
    {"role": "user", "type": "pageText", "content": ")" +
                                expected_page_content_1 + R"("},
    {"role": "user", "type": "chatMessage", "content": ")" +
                                expected_user_message_content + R"("}
  ])";
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Some structured EXPECT calls to catch nicer errors first
        EXPECT_EQ(conversation.size(), 3u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[1].type, ConversationEventType::kPageText);
        EXPECT_EQ(GetContentStrings(conversation[0].content)[0],
                  expected_page_content_2);
        EXPECT_EQ(GetContentStrings(conversation[1].content)[0],
                  expected_page_content_1);
        EXPECT_EQ(conversation[2].role, ConversationEventRole::kUser);
        // Match entire structure
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events));
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
      {{{"turn-1", {page_content_1, page_content_2}}}}, history, "", false, {},
      std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_WithSelectedText) {
  PageContent page_content("This is a page about The Mandalorian.", false);
  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": "This is a page about The Mandalorian."},
    {"role": "user", "type": "pageExcerpt", "content": "The Mandalorian"},
    {"role": "user", "type": "chatMessage", "content": "Is this related to a broader series?"}
  ])";
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Some structured EXPECT calls to catch nicer errors first
        EXPECT_EQ(conversation.size(), 3u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[1].type, ConversationEventType::kPageExcerpt);
        EXPECT_EQ(conversation[2].role, ConversationEventRole::kUser);
        // Match entire structure
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events));
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
      {{{"turn-1", {page_content}}}}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateEvents_HistoryWithSelectedText) {
  PageContent page_content("This is my page. I have spoken.", false);
  // Tests events building from history with selected text and new query without
  // selected text but with page association.
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
  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": "This is my page. I have spoken."},
    {"role": "user", "type": "pageExcerpt", "content": "I have spoken."},
    {"role": "user", "type": "chatMessage", "content": "Which show is this catchphrase from?"},
    {"role": "assistant", "type": "chatMessage", "content": "The Mandalorian."},
    {"role": "user", "type": "chatMessage", "content": "Is it related to a broader series?"}
  ])";
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Some structured EXPECT calls to catch nicer errors first
        EXPECT_EQ(conversation.size(), 5u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[2].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[3].role, ConversationEventRole::kAssistant);
        EXPECT_EQ(conversation[4].role, ConversationEventRole::kUser);
        // Match entire JSON
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });
  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content}}}}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_ModifyReply) {
  // Tests events building from history with modified agent reply.
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
  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": "I have spoken."},
    {"role": "user", "type": "chatMessage",
     "content": "Which show is 'This is the way' from?"},
    {"role": "assistant", "type": "chatMessage", "content": "The Mandalorian."},
    {"role": "user", "type": "chatMessage",
     "content": "Is it related to a broader series?"}
  ])";
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Some structured EXPECT calls to catch nicer errors first
        ASSERT_EQ(conversation.size(), 4u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[2].role, ConversationEventRole::kAssistant);
        EXPECT_EQ(conversation[3].role, ConversationEventRole::kUser);
        // Match entire JSON
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });
  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content}}}}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_SummarizePage) {
  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": "This is a sample page content."},
    {"role": "user", "type": "requestSummary", "content": ""}
  ])";
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Match entire structure to ensure the generated JSON is correct
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events));
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
  turn->text =
      "Summarize the content of this page.";  // This text should be ignored
  history.push_back(std::move(turn));
  PageContent page_content("This is a sample page content.", false);
  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content}}}}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_WithModelKeyOverride) {
  auto* mock_api_client = GetMockConversationAPIClient();
  constexpr char kModelKey[] = "chat-basic";

  // Expect PerformRequest with the overridden model name
  EXPECT_CALL(
      *mock_api_client,
      PerformRequest(_, _, _, _, _, _, _,
                     testing::Eq(std::optional<std::string>(
                         model_service_->GetLeoModelNameByKey(kModelKey)))))
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
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
      std::move(page_contents), std::move(history), "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_WithEmptyToolDefinitions) {
  // Verify we're not passing tools if we don't have any
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client,
              PerformRequest(_, _, testing::Eq(std::nullopt), _, _, _, _, _))
      .WillOnce([&]() { run_loop.Quit(); });

  auto history = CreateSampleChatHistory(2);

  engine_->GenerateAssistantResponse(
      {}, std::move(history), "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_WithToolDefinitions) {
  // Verify we're passing json-converted tool definitions.
  // For more variation tests, see oai_parsing_unittest.cc
  auto* mock_api_client = GetMockConversationAPIClient();
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

  EXPECT_CALL(
      *mock_api_client,
      PerformRequest(_, _,
                     testing::Optional(base::test::IsJson(expected_tools_json)),
                     _, _, _, _, _))
      .WillOnce([&]() { run_loop.Quit(); });

  auto history = CreateSampleChatHistory(2);

  engine_->GenerateAssistantResponse(
      {}, std::move(history), "", false, {mock_tool->GetWeakPtr()},
      std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_PageContentsOrderedBeforeTurns) {
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Verify conversation structure: page content should come before
        // associated turn
        EXPECT_GE(conversation.size(), 2u);

        // First event should be page content for turn-1
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(GetContentStrings(conversation[0].content)[0],
                  "Test page content");

        // Second event should be the human turn
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(GetContentStrings(conversation[1].content)[0],
                  "Human message");

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
      {{"turn-1", {page_content}}}, std::move(history), "", false, {},
      std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&](EngineConsumer::
                  GenerationResult) { /* callback handled above */ }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_PageContentsExcludedForMissingTurns) {
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should only have the human turn, no page content for missing turn
        EXPECT_EQ(conversation.size(), 1u);

        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(GetContentStrings(conversation[0].content)[0],
                  "Human message");

        // Verify no page content was included
        EXPECT_TRUE(std::ranges::all_of(conversation, [](const auto& event) {
          return event.type != ConversationEventType::kPageText;
        }));

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
      {{"missing-turn", {page_content}}}, std::move(history), "", false, {},
      std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&](EngineConsumer::
                  GenerationResult) { /* callback handled above */ }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_MultiplePageContentsForSameTurn) {
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have both page contents before the human turn
        EXPECT_GE(conversation.size(), 3u);

        // First event should be video content
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type,
                  ConversationEventType::kVideoTranscript);
        EXPECT_EQ(GetContentStrings(conversation[0].content)[0],
                  "Video content");

        // Second event should be page content
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[1].type, ConversationEventType::kPageText);
        EXPECT_EQ(GetContentStrings(conversation[1].content)[0],
                  "First page content");

        // Third event should be the human turn
        EXPECT_EQ(conversation[2].role, ConversationEventRole::kUser);
        EXPECT_EQ(GetContentStrings(conversation[2].content)[0],
                  "Human message");

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
      {{"turn-1", {page_content1, video_content}}}, std::move(history), "",
      false, {}, std::nullopt, mojom::ConversationCapability::CHAT,
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&](EngineConsumer::
                  GenerationResult) { /* callback handled above */ }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_MultiTurnConversationWithPageContents) {
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Expected order:
        // 1. Page content for turn-1
        // 2. Human turn-1
        // 3. Assistant response-1
        // 4. Page content for turn-2
        // 5. Human turn-2
        EXPECT_EQ(conversation.size(), 5u);

        // Check ordering
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(GetContentStrings(conversation[0].content)[0],
                  "Content for first turn");
        EXPECT_EQ(GetContentStrings(conversation[1].content)[0],
                  "First human message");
        EXPECT_EQ(GetContentStrings(conversation[2].content)[0],
                  "First assistant response");
        EXPECT_EQ(GetContentStrings(conversation[3].content)[0],
                  "Content for second turn");
        EXPECT_EQ(GetContentStrings(conversation[4].content)[0],
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
      std::move(history), "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&](EngineConsumer::
                  GenerationResult) { /* callback handled above */ }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateEvents_MultiplePageContents_MultipleTurns_TooLong) {
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

  auto* mock_api_client = GetMockConversationAPIClient();
  auto test_content_truncation = [&](uint32_t max_length,
                                     std::vector<std::string> event_contents) {
    SCOPED_TRACE(
        absl::StrFormat("Testing Truncation with max length: %d", max_length));
    engine_->SetMaxAssociatedContentLengthForTesting(max_length);

    base::RunLoop run_loop;
    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& selected_language,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          EXPECT_EQ(conversation.size(), event_contents.size());
          for (size_t i = 0; i < event_contents.size(); ++i) {
            SCOPED_TRACE(absl::StrFormat("Checking event %zu (max length: %d)",
                                         i, max_length));
            EXPECT_EQ(GetContentStrings(conversation[i].content)[0],
                      event_contents[i]);
          }
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New(""));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
          run_loop.Quit();
        });

    engine_->GenerateAssistantResponse(
        {{"turn-1", {page_content_1, page_content_2}},
         {"turn-2", {page_content_3}}},
        history, "", false, {}, std::nullopt,
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

  // Test case: Max Length = 100 (should include all of content 3, all of
  // content 2, partial content 1) Content 3: 35 + Content 2: 35 + Content 1: 30
  // chars = 100 chars exactly
  test_content_truncation(100, {
                                   std::string(35, '2'),
                                   std::string(30, '1'),
                                   "Human message 1",
                                   std::string(35, '3'),
                                   "Human message 2",
                               });

  // Test case: Max Length = 70 (should include page content 3 + page content 2,
  // page content 1 gets omitted) Content 3: 35 chars + Content 2: 35 chars = 70
  // chars exactly
  test_content_truncation(70, {
                                  std::string(35, '2'),
                                  "Human message 1",
                                  std::string(35, '3'),
                                  "Human message 2",
                              });

  // Test case: Max Length = 65 (should include all of content 3, most of
  // content 2, omit content 1) Content 3: 35 + Content 2: 30 chars = 65 chars
  // exactly
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

  // Test case: Max Length = 10 (should include only partial content 3, omit
  // others)
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
       GenerateRewriteSuggestion_UnsupportedActionTypeReturnsInternalError) {
  auto* client = GetMockConversationAPIV2Client();

  // Expect PerformRequest is NOT called for unsupported action types
  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _, _, _, _)).Times(0);

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateRewriteSuggestion("Hello World",
                                     mojom::ActionType::CREATE_TAGLINE, "",
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
  if (params.expected_content_type == ExtendedContentBlockType::kChangeTone) {
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

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _, _, _, _))
      .WillOnce(
          [&](std::vector<OAIMessage> messages,
              const std::string& selected_language,
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
            EXPECT_EQ(first_message.content[0].type,
                      ExtendedContentBlockType::kPageExcerpt);
            EXPECT_EQ(std::get<TextContent>(first_message.content[0].data).text,
                      test_text);

            // Second content block should be the action type
            EXPECT_EQ(first_message.content[1].type,
                      params.expected_content_type);

            // Verify the content data, should have tone for change tone type,
            // empty text otherwise.
            if (params.expected_content_type ==
                ExtendedContentBlockType::kChangeTone) {
              auto* tone_content = std::get_if<ChangeToneContent>(
                  &first_message.content[1].data);
              ASSERT_TRUE(tone_content);
              EXPECT_EQ(tone_content->tone, params.expected_payload);
            } else {
              auto* text_content =
                  std::get_if<TextContent>(&first_message.content[1].data);
              ASSERT_TRUE(text_content);
              EXPECT_EQ(text_content->text, params.expected_payload);
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
      test_text, params.action_type, "", base::DoNothing(),
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
        GenerateRewriteTestParam{"Paraphrase", mojom::ActionType::PARAPHRASE,
                                 ExtendedContentBlockType::kParaphrase, "",
                                 "brave-request-paraphrase"},
        GenerateRewriteTestParam{"Improve", mojom::ActionType::IMPROVE,
                                 ExtendedContentBlockType::kImprove, "",
                                 "brave-request-improve-excerpt-language"},
        GenerateRewriteTestParam{"Shorten", mojom::ActionType::SHORTEN,
                                 ExtendedContentBlockType::kShorten, "",
                                 "brave-request-shorten"},
        GenerateRewriteTestParam{"Expand", mojom::ActionType::EXPAND,
                                 ExtendedContentBlockType::kExpand, "",
                                 "brave-request-expansion"},
        GenerateRewriteTestParam{"Academic", mojom::ActionType::ACADEMICIZE,
                                 ExtendedContentBlockType::kChangeTone,
                                 "academic", "brave-request-change-tone"},
        GenerateRewriteTestParam{"Professional",
                                 mojom::ActionType::PROFESSIONALIZE,
                                 ExtendedContentBlockType::kChangeTone,
                                 "professional", "brave-request-change-tone"},
        GenerateRewriteTestParam{"Casual", mojom::ActionType::CASUALIZE,
                                 ExtendedContentBlockType::kChangeTone,
                                 "casual", "brave-request-change-tone"},
        GenerateRewriteTestParam{"Funny", mojom::ActionType::FUNNY_TONE,
                                 ExtendedContentBlockType::kChangeTone, "funny",
                                 "brave-request-change-tone"},
        GenerateRewriteTestParam{"Persuasive",
                                 mojom::ActionType::PERSUASIVE_TONE,
                                 ExtendedContentBlockType::kChangeTone,
                                 "persuasive", "brave-request-change-tone"}),
    [](const testing::TestParamInfo<GenerateRewriteTestParam>& info) {
      return info.param.name;
    });

}  // namespace ai_chat

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/base64.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/to_string.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/origin.h"

using ::testing::_;

namespace ai_chat {

namespace {

constexpr int kTestingMaxAssociatedContentLength = 100;
constexpr size_t kChunkSize = 75;

std::pair<std::vector<Tab>, std::vector<std::string>>
GetMockTabsAndExpectedTabsJsonString(size_t num_tabs) {
  size_t num_chunks = (num_tabs + kChunkSize - 1) / kChunkSize;
  std::vector<Tab> tabs;
  std::vector<std::string> tabs_json_strings;
  for (size_t i = 0; i < num_chunks; i++) {
    std::string tabs_json_string = "[";
    size_t start_suffix = i * kChunkSize;
    for (size_t j = start_suffix;
         j < std::min(kChunkSize + start_suffix, num_tabs); j++) {
      std::string id = base::StrCat({"id", base::NumberToString(j)});
      std::string title = base::StrCat({"title", base::NumberToString(j)});
      std::string url = base::StrCat(
          {"https://www.example", base::NumberToString(j), ".com"});
      tabs.push_back({id, title, url::Origin::Create(GURL(url))});
      base::StrAppend(&tabs_json_string,
                      {R"({\"id\":\")", id, R"(\",\"title\":\")", title,
                       R"(\",\"url\":\")", url, R"(\"},)"});
    }

    if (!tabs_json_string.empty() && tabs_json_string.back() == ',') {
      tabs_json_string.pop_back();  // Remove comma
    }
    base::StrAppend(&tabs_json_string, {"]"});
    tabs_json_strings.push_back(tabs_json_string);
  }
  return {tabs, tabs_json_strings};
}

// Helper function to create base64 data URL from uploaded file data
std::string CreateDataURLFromUploadedFile(const mojom::UploadedFilePtr& file,
                                          const std::string& mime_type) {
  std::string base64_data = base::Base64Encode(file->data);
  return absl::StrFormat("data:%s;base64,%s", mime_type, base64_data);
}

}  // namespace

using ConversationEvent = ConversationAPIClient::ConversationEvent;
using ConversationEventRole = ConversationAPIClient::ConversationEventRole;
using ConversationEventType = ConversationAPIClient::ConversationEventType;

class MockConversationAPIClient : public ConversationAPIClient {
 public:
  explicit MockConversationAPIClient(const std::string& model_name)
      : ConversationAPIClient(model_name, nullptr, nullptr, nullptr) {}
  ~MockConversationAPIClient() override = default;

  MOCK_METHOD(void,
              PerformRequest,
              (std::vector<ConversationEvent>,
               const std::string& selected_language,
               std::optional<base::Value::List> oai_tool_definitions,
               const std::optional<std::string>& preferred_tool_name,
               mojom::ConversationCapability conversation_capability,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback,
               const std::optional<std::string>& model_name),
              (override));

  std::string GetEventsJson(std::vector<ConversationEvent> conversation) {
    auto body = CreateJSONRequestBody(
        std::move(conversation), "", std::nullopt, std::nullopt,
        mojom::ConversationCapability::CHAT, std::nullopt, true);
    auto dict = base::test::ParseJsonDict(body);
    base::Value::List* events = dict.FindList("events");
    EXPECT_TRUE(events);
    std::string events_json;
    base::JSONWriter::WriteWithOptions(
        *events, base::JSONWriter::OPTIONS_PRETTY_PRINT, &events_json);
    return events_json;
  }
};

class EngineConsumerConversationAPIUnitTest : public testing::Test {
 public:
  EngineConsumerConversationAPIUnitTest() = default;
  ~EngineConsumerConversationAPIUnitTest() override = default;

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

    engine_ = std::make_unique<EngineConsumerConversationAPI>(
        *model_->options->get_leo_model_options(), nullptr, nullptr,
        model_service_.get(), &prefs_);
    engine_->SetAPIForTesting(std::make_unique<MockConversationAPIClient>(
        model_->options->get_leo_model_options()->name));
  }

  MockConversationAPIClient* GetMockConversationAPIClient() {
    return static_cast<MockConversationAPIClient*>(engine_->GetAPIForTesting());
  }

  void TearDown() override {}

  std::string FormatComparableEventsJson(std::string_view formatted_json) {
    auto events = base::test::ParseJson(formatted_json);
    std::string events_json;
    base::JSONWriter::WriteWithOptions(
        events, base::JSONWriter::OPTIONS_PRETTY_PRINT, &events_json);
    return events_json;
  }

  std::vector<std::string> GetContentStrings(
      ConversationAPIClient::Content& content) {
    auto* content_strings = std::get_if<std::vector<std::string>>(&content);

    EXPECT_TRUE(content_strings);
    return *content_strings;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  mojom::ModelPtr model_;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<EngineConsumerConversationAPI> engine_;
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
      std::nullopt, nullptr /* smart_mode */, false,
      std::nullopt /* model_key */));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "The Mandalorian.", std::nullopt /* prompt */, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt /* model_key */));
  history.push_back(mojom::ConversationTurn::New(
      "turn-3", mojom::CharacterType::HUMAN, mojom::ActionType::RESPONSE,
      "Is it related to a broader series?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* smart_mode */, false, std::nullopt /* model_key */));
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

struct RewriteTestData {
  std::string name;
  mojom::ActionType action_type;
  std::string expected_type;
  std::string expected_tone;
};

class EngineConsumerConversationAPIUnitTest_Rewrite
    : public EngineConsumerConversationAPIUnitTest,
      public testing::WithParamInterface<RewriteTestData> {};

TEST_P(EngineConsumerConversationAPIUnitTest_Rewrite, GenerateEvents) {
  const RewriteTestData& test_data = GetParam();

  std::string expected_events;
  if (test_data.expected_tone.empty()) {
    expected_events = absl::StrFormat(
        R"([
          {"role": "user", "type": "pageExcerpt", "content": "Hello World"},
          {"role": "user", "type": "%s", "content": ""}
        ])",
        test_data.expected_type);
  } else {
    expected_events = absl::StrFormat(
        R"([
          {"role": "user", "type": "pageExcerpt", "content": "Hello World"},
          {"role": "user", "type": "%s", "content": "", "tone": "%s"}
        ])",
        test_data.expected_type, test_data.expected_tone);
  }

  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIClient();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 2u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateRewriteSuggestion(
      "Hello World", test_data.action_type, "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

INSTANTIATE_TEST_SUITE_P(
    AllRewriteTypes,
    EngineConsumerConversationAPIUnitTest_Rewrite,
    testing::Values(RewriteTestData{"Paraphrase", mojom::ActionType::PARAPHRASE,
                                    "requestParaphrase", ""},
                    RewriteTestData{"Improve", mojom::ActionType::IMPROVE,
                                    "requestImprove", ""},
                    RewriteTestData{"Shorten", mojom::ActionType::SHORTEN,
                                    "requestShorten", ""},
                    RewriteTestData{"Expand", mojom::ActionType::EXPAND,
                                    "requestExpand", ""},
                    RewriteTestData{"Academic", mojom::ActionType::ACADEMICIZE,
                                    "requestChangeTone", "academic"},
                    RewriteTestData{"Professional",
                                    mojom::ActionType::PROFESSIONALIZE,
                                    "requestChangeTone", "professional"},
                    RewriteTestData{"Casual", mojom::ActionType::CASUALIZE,
                                    "requestChangeTone", "casual"},
                    RewriteTestData{"Funny", mojom::ActionType::FUNNY_TONE,
                                    "requestChangeTone", "funny"},
                    RewriteTestData{"Persuasive",
                                    mojom::ActionType::PERSUASIVE_TONE,
                                    "requestChangeTone", "persuasive"}),
    [](const testing::TestParamInfo<RewriteTestData>& info) {
      return info.param.name;
    });

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_ToolUse) {
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "What is the weather in Santa Barbara?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* smart_mode */, false, std::nullopt /* model_key */));

  std::vector<mojom::ContentBlockPtr> tool_output_content_blocks;
  tool_output_content_blocks.push_back(mojom::ContentBlock::NewTextContentBlock(
      mojom::TextContentBlock::New("{ \"temperature\":\"75F\" }")));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("First I'll look up the weather...")));
  response_events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_123", "{\"location\":\"Santa Barbara\"}",
          std::move(tool_output_content_blocks))));

  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "First I'll look up the weather...", std::nullopt /* prompt */,
      std::nullopt, std::move(response_events), base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* smart_mode */, false,
      std::nullopt /* model_key */));

  std::string expected_events = R"([
    {
      "role": "user",
      "type": "chatMessage",
      "content": "What is the weather in Santa Barbara?"
    },
    {
      "role": "assistant",
      "type": "toolCalls",
      "content": "First I'll look up the weather...",
      "tool_calls": [
        {
          "id": "call_123",
          "type": "function",
          "function": {
            "name": "get_weather",
            "arguments": "{\"location\":\"Santa Barbara\"}",
          }
        },
      ],
    },
    {
      "role": "tool",
      "type": "toolUse",
      "tool_call_id": "call_123",
      "content": [{"type": "text", "text": "{ \"temperature\":\"75F\" }"}],
    },
  ])";
  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIClient();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // One user turn, one assistant turn, one tool turn
        EXPECT_EQ(conversation.size(), 3u);
        EXPECT_THAT(mock_api_client->GetEventsJson(std::move(conversation)),
                    base::test::IsJson(base::test::ParseJson(expected_events)));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_MultipleToolUse) {
  // Responses can contain multiple tool use events
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "What is the weather in Santa Barbara?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* smart_mode */, false, std::nullopt /* model_key */));

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
          std::move(temperature_tool_output_content_blocks))));

  std::vector<mojom::ContentBlockPtr> wind_tool_output_content_blocks;
  wind_tool_output_content_blocks.push_back(
      mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
          "{ \"speed\":\"25mph\", \"direction\":\"NW\" }")));
  response_events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_wind", "call_1234", "{\"location\":\"Santa Barbara\"}",
          std::move(wind_tool_output_content_blocks))));

  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "First I'll look up the weather...", std::nullopt /* prompt */,
      std::nullopt, std::move(response_events), base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* smart_mode */, false,
      std::nullopt /* model_key */));

  std::string expected_events = R"([
    {
      "role": "user",
      "type": "chatMessage",
      "content": "What is the weather in Santa Barbara?"
    },
    {
      "role": "assistant",
      "type": "toolCalls",
      "content": "First I'll look up the weather...",
      "tool_calls": [
        {
          "id": "call_123",
          "type": "function",
          "function": {
            "name": "get_temperature",
            "arguments": "{\"location\":\"Santa Barbara\"}",
          }
        },
        {
          "id": "call_1234",
          "type": "function",
          "function": {
            "name": "get_wind",
            "arguments": "{\"location\":\"Santa Barbara\"}",
          }
        },
      ],
    },
    {
      "role": "tool",
      "type": "toolUse",
      "tool_call_id": "call_123",
      "content": [{"type": "text", "text": "{ \"temperature\":\"75F\" }"}],
    },
    {
      "role": "tool",
      "type": "toolUse",
      "tool_call_id": "call_1234",
      "content": [
        {
          "type": "text",
          "text": "{ \"speed\":\"25mph\", \"direction\":\"NW\" }"
        }
      ],
    },
  ])";
  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIClient();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // One user turn, one assistant turn, two tool turns
        EXPECT_EQ(conversation.size(), 4u);
        EXPECT_THAT(base::test::ParseJson(mock_api_client->GetEventsJson(
                        std::move(conversation))),
                    base::test::IsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateEvents_MultipleToolUseWithLargeContent) {
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
        base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
        false, std::nullopt /* model_key */));
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
            "{}", std::move(tool_output_content_blocks))));
    history.push_back(mojom::ConversationTurn::New(
        "turn-" + base::NumberToString(i * 3 + 1),
        mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        "First I'll look up the page...", std::nullopt /* prompt */,
        std::nullopt, std::move(response_events), base::Time::Now(),
        std::nullopt, std::nullopt, nullptr /* smart_mode */, false,
        std::nullopt /* model_key */));
    history.push_back(mojom::ConversationTurn::New(
        "turn-" + base::NumberToString(i * 3 + 2),
        mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        "The page has some great content", std::nullopt /* prompt */,
        std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
        std::nullopt, nullptr /* smart_mode */, false,
        std::nullopt /* model_key */));
  }

  std::string expected_events = R"([
    {
      "role": "user",
      "type": "chatMessage",
      "content": "What is this web page about?"
    },
    {
      "role": "assistant",
      "type": "toolCalls",
      "content": "First I'll look up the page...",
      "tool_calls": [
        {
          "id": "call_1230",
          "type": "function",
          "function": {
            "name": "get_page_content",
            "arguments": "{}",
          }
        },
      ],
    },
    {
      "role": "tool",
      "type": "toolUse",
      "tool_call_id": "call_1230",
      "content": "[Large result removed to save space for subsequent results]",
    },
    {
      "role": "assistant",
      "type": "chatMessage",
      "content": "The page has some great content"
    },

    {
      "role": "user",
      "type": "chatMessage",
      "content": "What is this web page about?"
    },
    {
      "role": "assistant",
      "type": "toolCalls",
      "content": "First I'll look up the page...",
      "tool_calls": [
        {
          "id": "call_1231",
          "type": "function",
          "function": {
            "name": "get_page_content",
            "arguments": "{}",
          }
        },
      ],
    },
    {
      "role": "tool",
      "type": "toolUse",
      "tool_call_id": "call_1231",
      "content": [{"type": "text", "text": ")" +
                                large_text_content + R"("}],
    },
    {
      "role": "assistant",
      "type": "chatMessage",
      "content": "The page has some great content"
    },

    {
      "role": "user",
      "type": "chatMessage",
      "content": "What is this web page about?"
    },
    {
      "role": "assistant",
      "type": "toolCalls",
      "content": "First I'll look up the page...",
      "tool_calls": [
        {
          "id": "call_1232",
          "type": "function",
          "function": {
            "name": "get_page_content",
            "arguments": "{}",
          }
        },
      ],
    },
    {
      "role": "tool",
      "type": "toolUse",
      "tool_call_id": "call_1232",
      "content": [
        { "type": "image_url",
          "image_url": { "url": "data:image/png;base64,ABC=" } }
      ],
    },
    {
      "role": "assistant",
      "type": "chatMessage",
      "content": "The page has some great content"
    },
  ])";
  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIClient();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_THAT(base::test::ParseJson(mock_api_client->GetEventsJson(
                        std::move(conversation))),
                    base::test::IsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_ToolUseNoOutput) {
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "What is the weather in Santa Barbara?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* smart_mode */, false, std::nullopt /* model_key */));

  std::vector<mojom::ConversationEntryEventPtr> response_events;
  response_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("First I'll look up the weather...")));
  response_events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "get_weather", "call_123", "{\"location\":\"Santa Barbara\"}",
          std::nullopt)));

  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "First I'll look up the weather...", std::nullopt /* prompt */,
      std::nullopt, std::move(response_events), base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* smart_mode */, false,
      std::nullopt /* model_key */));

  // If somehow the conversation is sent without the tool output, the
  // request should not include the tool request, since most LLM APIs will fail
  // in that scenario. This should be prevented by the callers.
  std::string expected_events = R"([
    {
      "role": "user",
      "type": "chatMessage",
      "content": "What is the weather in Santa Barbara?"
    },
    {
      "role": "assistant",
      "type": "chatMessage",
      "content": "First I'll look up the weather..."
    }
  ])";
  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIClient();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 2u);
        EXPECT_THAT(mock_api_client->GetEventsJson(std::move(conversation)),
                    base::test::IsJson(base::test::ParseJson(expected_events)));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, history, "", false, {}, std::nullopt,
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
      nullptr /* smart_mode */, false, std::nullopt /* model_key */));

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
      nullptr /* smart_mode */, false, std::nullopt /* model_key */);
  std::vector<mojom::ConversationTurnPtr> edits;
  edits.push_back(std::move(edit));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Mandalorian.", std::nullopt /* prompt */, std::nullopt,
      std::move(events), base::Time::Now(), std::move(edits), std::nullopt,
      nullptr /* smart_mode */, false, std::nullopt /* model_key */));
  history.push_back(mojom::ConversationTurn::New(
      "turn-3", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Is it related to a broader series?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* smart_mode */, false, std::nullopt /* model_key */));
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

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_UploadImage) {
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
        ASSERT_EQ(conversation.size(), 3u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        for (size_t i = 0; i < 3; ++i) {
          EXPECT_EQ(
              GetContentStrings(conversation[0].content)[i],
              base::StrCat({"data:image/png;base64,",
                            base::Base64Encode(uploaded_images[i]->data)}));
        }
        EXPECT_EQ(conversation[0].type, ConversationEventType::kUploadImage);
        for (size_t i = 3; i < uploaded_images.size(); ++i) {
          EXPECT_EQ(
              GetContentStrings(conversation[1].content)[i - 3],
              base::StrCat({"data:image/png;base64,",
                            base::Base64Encode(uploaded_images[i]->data)}));
        }
        EXPECT_EQ(conversation[1].type, ConversationEventType::kPageScreenshot);
        EXPECT_EQ(conversation[2].role, ConversationEventRole::kUser);
        EXPECT_EQ(GetContentStrings(conversation[2].content)[0], kTestPrompt);
        EXPECT_EQ(conversation[2].type, ConversationEventType::kChatMessage);
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
      nullptr /* smart_mode */, false, std::nullopt /* model_key */));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateAssistantResponse({}, history, "", false, {}, std::nullopt,
                                     mojom::ConversationCapability::CHAT,
                                     base::DoNothing(), future.GetCallback());
  EXPECT_EQ(future.Take(),
            EngineConsumer::GenerationResultData(
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New(kAssistantResponse)),
                std::nullopt /* model_key */));
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GetSuggestedTopics) {
  auto [tabs, tabs_json_strings] =
      GetMockTabsAndExpectedTabsJsonString(2 * kChunkSize);
  ASSERT_EQ(tabs.size(), 2 * kChunkSize);
  ASSERT_EQ(tabs_json_strings.size(), 2u);

  std::string expected_events1 = R"([
    {"role": "user", "type": "suggestFocusTopics", "content": ")" +
                                 tabs_json_strings[0] + R"("}])";
  std::string expected_events2 = R"([
    {"role": "user", "type": "suggestFocusTopics", "content": ")" +
                                 tabs_json_strings[1] + R"("}])";
  std::string expected_events3 = R"([
    {"role": "user", "type": "dedupeFocusTopics", "content": "[\"topic1\",\"topic2\",\"topic3\",\"topic7\",\"topic3\",\"topic4\",\"topic5\",\"topic6\"]"}])";

  auto* mock_api_client = GetMockConversationAPIClient();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(3)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic3\", \"topic4\", \"topic5\", "
                    "\"topic6\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events3));
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
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events2));
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
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic3\", \"topic4\", \"topic5\", "
                    "\"topic6\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events3));
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
  std::string expected_events3_skipped_invalid_response = R"([
    {"role": "user", "type": "dedupeFocusTopics", "content": "[\"topic1\",\"topic2\",\"topic3\",\"topic7\"]"}])";
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(3)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("not well structured"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(
                      expected_events3_skipped_invalid_response));
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
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic3\", \"topic4\", \"topic5\", "
                    "\"topic6\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events3));
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
      .WillRepeatedly([&](std::vector<ConversationEvent> conversation,
                          const std::string& selected_language,
                          std::optional<base::Value::List> oai_tool_definitions,
                          const std::optional<std::string>& preferred_tool_name,
                          mojom::ConversationCapability conversation_capability,
                          EngineConsumer::GenerationDataCallback data_callback,
                          EngineConsumer::GenerationCompletedCallback callback,
                          const std::optional<std::string>& model_name) {
        ASSERT_EQ(conversation.size(), 1u);
        EXPECT_EQ(conversation[0].type,
                  ConversationEventType::kGetSuggestedTopicsForFocusTabs);
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

TEST_F(EngineConsumerConversationAPIUnitTest,
       GetSuggestedTopics_SingleTabChunk) {
  auto [tabs, tabs_json_strings] = GetMockTabsAndExpectedTabsJsonString(1);
  ASSERT_EQ(tabs.size(), 1u);
  ASSERT_EQ(tabs_json_strings.size(), 1u);

  std::string expected_events = R"([
    {"role": "user", "type": "suggestAndDedupeFocusTopics", "content": ")" +
                                tabs_json_strings[0] + R"("}])";

  auto* mock_api_client = GetMockConversationAPIClient();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\"] }"));
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

TEST_F(EngineConsumerConversationAPIUnitTest, GetFocusTabs) {
  // Get two full chunks of tabs for testing.
  auto [tabs, tabs_json_strings] =
      GetMockTabsAndExpectedTabsJsonString(2 * kChunkSize);
  ASSERT_EQ(tabs.size(), 2 * kChunkSize);
  ASSERT_EQ(tabs_json_strings.size(), 2u);

  std::string expected_events1 = R"([
    {"role": "user", "type": "classifyTabs", "content": ")" +
                                 tabs_json_strings[0] +
                                 R"(", "topic": "test_topic"}
  ])";
  std::string expected_events2 = R"([
    {"role": "user", "type": "classifyTabs", "content": ")" +
                                 tabs_json_strings[1] +
                                 R"(", "topic": "test_topic"}
  ])";

  auto* mock_api_client = GetMockConversationAPIClient();
  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(2)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id1\", \"id2\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id75\", \"id76\"] }"));
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
      GetMockTabsAndExpectedTabsJsonString(kChunkSize + 5);
  ASSERT_EQ(tabs2.size(), kChunkSize + 5);
  ASSERT_EQ(tabs_json_strings2.size(), 2u);

  expected_events1 = R"([
    {"role": "user", "type": "classifyTabs", "content": ")" +
                     tabs_json_strings2[0] + R"(", "topic": "test_topic2"}
  ])";
  expected_events2 = R"([
    {"role": "user", "type": "classifyTabs", "content": ")" +
                     tabs_json_strings2[1] + R"(", "topic": "test_topic2"}
  ])";

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .Times(2)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id3\", \"id5\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events2));
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
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id3\", \"id5\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
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
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id3\", \"id5\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
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

TEST_F(EngineConsumerConversationAPIUnitTest, GetStrArrFromResponse) {
  std::vector<EngineConsumer::GenerationResult> results;
  EXPECT_EQ(
      EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
          results),
      base::unexpected(mojom::APIError::InternalError));

  auto add_result = [&results](const std::string& completion_text) {
    results.push_back(base::ok(EngineConsumer::GenerationResultData(
        mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(completion_text)),
        std::nullopt)));
  };

  // Test specifically the "Skip empty results" code path
  results.clear();

  // This creates a result with an event that is not a completion event
  results.push_back(base::ok(EngineConsumer::GenerationResultData(
      mojom::ConversationEntryEvent::NewSelectedLanguageEvent(
          mojom::SelectedLanguageEvent::New("en-us")),
      std::nullopt)));

  // This creates a result with no event
  results.push_back(
      base::ok(EngineConsumer::GenerationResultData(nullptr, std::nullopt)));

  // This creates a result with an empty completion
  results.push_back(base::ok(EngineConsumer::GenerationResultData(
      mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New("")),
      std::nullopt)));

  // Add a valid result
  add_result("[\"validString\"]");

  // Verify the empty results are skipped and we get only the valid string
  EXPECT_EQ(
      EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
          results),
      std::vector<std::string>({"validString"}));

  // Test with an empty vector
  results.clear();
  EXPECT_EQ(
      EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
          results),
      base::unexpected(mojom::APIError::InternalError));

  // Test with only one invalid result
  add_result("   ");
  EXPECT_EQ(
      EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
          results),
      base::unexpected(mojom::APIError::InternalError));

  // Test only valid strings are added to the result
  add_result("null");
  add_result("[]");
  add_result("[   ]");
  add_result("[null]");
  add_result("[\"\"]");
  add_result("[1, 2, 3]");
  add_result("[\"string1\", \"string2\", \"string3\"]");
  add_result(
      "Result\n: [\"\xF0\x9F\x98\x8A string4\", \"string5\", \"string6\"] "
      "TEST");
  add_result("[{[\"string7\", \"string8\", \"string9\"]}]");

  EXPECT_EQ(
      EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
          results),
      std::vector<std::string>({"string1", "string2", "string3",
                                "\xF0\x9F\x98\x8A string4", "string5",
                                "string6"}));

  // Test having a error message inside the response
  results.clear();
  add_result("[\"string1\", \"string2\", \"string3\"]");
  results.push_back(base::unexpected(mojom::APIError::RateLimitReached));
  EXPECT_EQ(
      EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
          results),
      base::unexpected(mojom::APIError::RateLimitReached));
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateQuestionSuggestions) {
  PageContent page_content("Sample page content.", false);
  PageContent video_content("Sample video content.", true);
  PageContents page_contents{page_content, video_content};

  std::string selected_language = "en-US";

  std::string expected_events = R"([
    {"role": "user", "type": "videoTranscript", "content": "Sample video content."},
    {"role": "user", "type": "pageText", "content": "Sample page content."},
    {"role": "user", "type": "requestSuggestedActions", "content": ""}
  ])";

  auto* mock_api_client = GetMockConversationAPIClient();

  // Test successful response
  {
    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& language,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          EXPECT_EQ(conversation.size(), 3u);
          EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                    FormatComparableEventsJson(expected_events));
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("question1|question2|question3"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    engine_->GenerateQuestionSuggestions(
        page_contents, selected_language,
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
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& language,
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
        page_contents, selected_language,
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
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& language,
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
        page_contents, selected_language,
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
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& language,
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
        page_contents, selected_language,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_FALSE(result.has_value());
          EXPECT_EQ(result.error(), mojom::APIError::InternalError);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test non-completion event
  {
    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& language,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          auto selected_language_event =
              mojom::ConversationEntryEvent::NewSelectedLanguageEvent(
                  mojom::SelectedLanguageEvent::New("en-us"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(selected_language_event), std::nullopt)));
        });

    engine_->GenerateQuestionSuggestions(
        page_contents, selected_language,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_FALSE(result.has_value());
          EXPECT_EQ(result.error(), mojom::APIError::InternalError);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_WithMemoryEvent) {
  auto* mock_api_client = GetMockConversationAPIClient();

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

    std::string expected_events = R"([
      {"content": "", "memory": {"name": "John Doe", "job": "Software Engineer",
       "tone": "Professional", "other": "Loves coding"}, "role": "user",
       "type": "userMemory"},
      {"role": "user", "type": "pageText",
       "content": "This is a test page content."},
      {"role": "user", "type": "chatMessage", "content": "What is this about?"}
    ])";

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& selected_language,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          EXPECT_EQ(conversation.size(), 3u);
          EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[0].type, ConversationEventType::kUserMemory);
          EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[1].type, ConversationEventType::kPageText);
          EXPECT_EQ(conversation[2].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[2].type, ConversationEventType::kChatMessage);
          EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                    FormatComparableEventsJson(expected_events));
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
        {{"turn-1", {page_content}}}, std::move(history), "", false, {},
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

    std::string expected_events = R"([
      {"content": "",
       "memory":{
         "memories": [
           "I prefer concise explanations",
           "I work in the tech industry"
         ]
       },
       "role": "user", "type": "userMemory"},
      {"role": "user", "type": "pageText",
       "content": "This is a test page content."},
      {"role": "user", "type": "chatMessage", "content": "What is this about?"}
    ])";

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& selected_language,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          EXPECT_EQ(conversation.size(), 3u);
          EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[0].type, ConversationEventType::kUserMemory);
          EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                    FormatComparableEventsJson(expected_events));
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
        {{"turn-1", {page_content}}}, std::move(history), "", false, {},
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

    std::string expected_events = R"([
      {"content": "",
       "memory": {
         "name": "Alice", "job": "Designer",
         "memories": ["I like creative solutions"]},
         "role": "user", "type": "userMemory"},
      {"role": "user", "type": "pageText",
       "content": "This is a test page content."},
      {"role": "user", "type": "chatMessage", "content": "What is this about?"}
    ])";

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& selected_language,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          EXPECT_EQ(conversation.size(), 3u);
          EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[0].type, ConversationEventType::kUserMemory);
          EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                    FormatComparableEventsJson(expected_events));
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
        {{"turn-1", {page_content}}}, std::move(history), "", false, {},
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

    std::string expected_events = R"([
      {"role": "user", "type": "pageText",
       "content": "This is a test page content."},
      {"role": "user", "type": "chatMessage", "content": "What is this about?"}
    ])";

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& selected_language,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          EXPECT_EQ(conversation.size(), 2u);
          EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
          EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[1].type, ConversationEventType::kChatMessage);
          EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                    FormatComparableEventsJson(expected_events));
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
        {{"turn-1", {page_content}}}, std::move(history), "", false, {},
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

    std::string expected_events = R"([
      {"role": "user", "type": "pageText",
       "content": "This is a test page content."},
      {"role": "user", "type": "chatMessage", "content": "What is this about?"}
    ])";

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& selected_language,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          EXPECT_EQ(conversation.size(), 2u);
          EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
          EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[1].type, ConversationEventType::kChatMessage);
          EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                    FormatComparableEventsJson(expected_events));
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
        {{"turn-1", {page_content}}}, std::move(history), "", false, {},
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

    std::string expected_events = R"([
      {"role": "user", "type": "pageText",
       "content": "This is a test page content."},
      {"role": "user", "type": "chatMessage", "content": "What is this about?"}
    ])";

    EXPECT_CALL(*mock_api_client, PerformRequest)
        .WillOnce([&](std::vector<ConversationEvent> conversation,
                      const std::string& selected_language,
                      std::optional<base::Value::List> oai_tool_definitions,
                      const std::optional<std::string>& preferred_tool_name,
                      mojom::ConversationCapability conversation_capability,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          EXPECT_EQ(conversation.size(), 2u);
          EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
          EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
          EXPECT_EQ(conversation[1].type, ConversationEventType::kChatMessage);
          EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                    FormatComparableEventsJson(expected_events));
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
        {{"turn-1", {page_content}}}, std::move(history), "", false, {},
        std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::GenerationResult) {
              run_loop.Quit();
            }));
    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_TemporaryChatExcludesMemory) {
  auto* mock_api_client = GetMockConversationAPIClient();

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

  // Expect NO memory event when is_temporary_chat=true
  std::string expected_events = R"([
    {"role": "user", "type": "pageText",
     "content": "This is a test page content."},
    {"role": "user", "type": "chatMessage", "content": "What is this about?"}
  ])";

  EXPECT_CALL(*mock_api_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should only have 2 events: page content and user message
        // NO memory event should be present
        EXPECT_EQ(conversation.size(), 2u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[1].type, ConversationEventType::kChatMessage);
        EXPECT_EQ(mock_api_client->GetEventsJson(std::move(conversation)),
                  FormatComparableEventsJson(expected_events));
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
      {{"turn-1", {page_content}}}, std::move(history), "",
      true,  // is_temporary_chat = true
      {}, std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
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
       ShouldCallSanitizeInputOnPageContent) {
  class MockConversationAPIEngineConsumer
      : public EngineConsumerConversationAPI {
   public:
    using EngineConsumerConversationAPI::EngineConsumerConversationAPI;
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
      std::make_unique<MockConversationAPIClient>(
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
        {{{"turn-1", {page_content_1, page_content_2}}}}, history, "", false,
        {}, std::nullopt, mojom::ConversationCapability::CHAT,
        base::DoNothing(), base::DoNothing());
    testing::Mock::VerifyAndClearExpectations(mock_engine_consumer.get());
  }

  // Calling GenerateQuestionSuggestions should call SanitizeInput
  {
    EXPECT_CALL(*mock_engine_consumer, SanitizeInput(page_content_1.content));
    EXPECT_CALL(*mock_engine_consumer, SanitizeInput(page_content_2.content));

    mock_engine_consumer->GenerateQuestionSuggestions(
        {page_content_1, page_content_2}, "", base::DoNothing());
    testing::Mock::VerifyAndClearExpectations(mock_engine_consumer.get());
  }
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateEvents_WithUploadedPdfFiles) {
  PageContent page_content("This is a page about The Mandalorian.", false);

  // Create test uploaded PDF files
  auto uploaded_files =
      CreateSampleUploadedFiles(2, mojom::UploadedFileType::kPdf);

  // Create expected base64 data URLs from the actual file data
  std::string pdf1_data_url =
      CreateDataURLFromUploadedFile(uploaded_files[0], "application/pdf");
  std::string pdf2_data_url =
      CreateDataURLFromUploadedFile(uploaded_files[1], "application/pdf");

  std::string expected_events = absl::StrFormat(R"([
    {"role": "user", "type": "pageText", "content": "This is a page about The Mandalorian."},
    {"role": "user", "type": "uploadPdf", "content": ["%s", "%s"]},
    {"role": "user", "type": "chatMessage", "content": "Can you analyze these PDFs?"}
  ])",
                                                pdf1_data_url, pdf2_data_url);

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
        // Verify conversation structure
        EXPECT_EQ(conversation.size(), 3u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[1].type, ConversationEventType::kUploadPdf);
        EXPECT_EQ(GetContentStrings(conversation[1].content).size(), 2u);
        EXPECT_EQ(conversation[2].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[2].type, ConversationEventType::kChatMessage);

        // Verify PDF content starts with expected data URL format
        for (const auto& pdf_content :
             GetContentStrings(conversation[1].content)) {
          EXPECT_TRUE(pdf_content.find("data:application/pdf;base64,") == 0);
        }

        // Match entire structure with exact content matching
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
  turn->text = "Can you analyze these PDFs?";
  turn->uploaded_files = std::move(uploaded_files);
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {{"turn-1", {page_content}}}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateEvents_WithMixedUploadedFiles) {
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

  // Create expected base64 data URLs from the actual file data
  std::string pdf_data_url =
      CreateDataURLFromUploadedFile(uploaded_files[0], "application/pdf");
  std::string image_data_url =
      CreateDataURLFromUploadedFile(uploaded_files[1], "image/png");
  std::string screenshot_data_url =
      CreateDataURLFromUploadedFile(uploaded_files[2], "image/png");

  std::string expected_events =
      absl::StrFormat(R"([
    {"role": "user", "type": "pageText", "content": "This is a page about The Mandalorian."},
    {"role": "user", "type": "uploadImage", "content": "%s"},
    {"role": "user", "type": "pageScreenshot", "content": "%s"},
    {"role": "user", "type": "uploadPdf", "content": "%s"},
    {"role": "user", "type": "chatMessage", "content": "Can you analyze these files?"}
  ])",
                      image_data_url, screenshot_data_url, pdf_data_url);

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
        // Verify conversation structure
        EXPECT_EQ(conversation.size(), 5u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[1].type, ConversationEventType::kUploadImage);
        EXPECT_EQ(GetContentStrings(conversation[1].content).size(), 1u);
        EXPECT_EQ(conversation[2].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[2].type, ConversationEventType::kPageScreenshot);
        EXPECT_EQ(GetContentStrings(conversation[2].content).size(), 1u);
        EXPECT_EQ(conversation[3].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[3].type, ConversationEventType::kUploadPdf);
        EXPECT_EQ(GetContentStrings(conversation[3].content).size(), 1u);
        EXPECT_EQ(conversation[4].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[4].type, ConversationEventType::kChatMessage);

        // Verify content formats
        EXPECT_TRUE(GetContentStrings(conversation[1].content)[0].find(
                        "data:image/png;base64,") == 0);
        EXPECT_TRUE(GetContentStrings(conversation[2].content)[0].find(
                        "data:image/png;base64,") == 0);
        EXPECT_TRUE(GetContentStrings(conversation[3].content)[0].find(
                        "data:application/pdf;base64,") == 0);

        // Match entire structure with exact content matching
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
  turn->text = "Can you analyze these files?";
  turn->uploaded_files = std::move(uploaded_files);
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {{"turn-1", {page_content}}}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_WithOnlyPdfFiles) {
  // Test case with only PDF files, no page content
  auto uploaded_files =
      CreateSampleUploadedFiles(1, mojom::UploadedFileType::kPdf);

  // Create expected base64 data URL from the actual file data
  std::string pdf_data_url =
      CreateDataURLFromUploadedFile(uploaded_files[0], "application/pdf");

  std::string expected_events = absl::StrFormat(R"([
    {"role": "user", "type": "uploadPdf", "content": "%s"},
    {"role": "user", "type": "chatMessage", "content": "What's in this PDF?"}
  ])",
                                                pdf_data_url);

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
        // Verify conversation structure
        EXPECT_EQ(conversation.size(), 2u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kUploadPdf);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[1].type, ConversationEventType::kChatMessage);
        EXPECT_EQ(GetContentStrings(conversation[1].content).size(), 1u);

        // Verify PDF content format
        EXPECT_TRUE(GetContentStrings(conversation[0].content)[0].find(
                        "data:application/pdf;base64,") == 0);

        // Match entire structure with exact content matching
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
  turn->text = "What's in this PDF?";
  turn->uploaded_files = std::move(uploaded_files);
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateEvents_WithMultiplePdfFiles) {
  PageContent page_content("This is a page about The Mandalorian.", false);

  // Create multiple PDF files
  auto uploaded_files =
      CreateSampleUploadedFiles(3, mojom::UploadedFileType::kPdf);

  // Create expected base64 data URLs from the actual file data
  std::string pdf1_data_url =
      CreateDataURLFromUploadedFile(uploaded_files[0], "application/pdf");
  std::string pdf2_data_url =
      CreateDataURLFromUploadedFile(uploaded_files[1], "application/pdf");
  std::string pdf3_data_url =
      CreateDataURLFromUploadedFile(uploaded_files[2], "application/pdf");

  std::string expected_events =
      absl::StrFormat(R"([
    {"role": "user", "type": "pageText", "content": "This is a page about The Mandalorian."},
    {"role": "user", "type": "uploadPdf", "content": ["%s", "%s", "%s"]},
    {"role": "user", "type": "chatMessage", "content": "Can you compare these three PDFs?"}
  ])",
                      pdf1_data_url, pdf2_data_url, pdf3_data_url);

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
        // Verify conversation structure
        EXPECT_EQ(conversation.size(), 3u);
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kPageText);
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[1].type, ConversationEventType::kUploadPdf);
        EXPECT_EQ(GetContentStrings(conversation[1].content).size(), 3u);
        EXPECT_EQ(conversation[2].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[2].type, ConversationEventType::kChatMessage);

        // Verify all PDF content formats
        for (const auto& pdf_content :
             GetContentStrings(conversation[1].content)) {
          EXPECT_TRUE(pdf_content.find("data:application/pdf;base64,") == 0);
        }

        // Match entire structure with exact content matching
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
  turn->text = "Can you compare these three PDFs?";
  turn->uploaded_files = std::move(uploaded_files);
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      {{"turn-1", {page_content}}}, history, "", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
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
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);
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
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);
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
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);
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
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);
  history.push_back(std::move(turn1));

  auto response1 = mojom::ConversationTurn::New(
      "response-1", mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "First assistant response", std::nullopt,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* smart_mode */, false, std::nullopt);
  history.push_back(std::move(response1));

  // Second turn
  auto turn2 = mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Second human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);
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
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);
  history.push_back(std::move(turn1));

  auto turn2 = mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Human message 2", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);
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

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_WithSmartMode) {
  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  // Create conversation history with smart mode entry
  EngineConsumer::ConversationHistory conversation_history;
  auto smart_mode_entry =
      mojom::SmartModeEntry::New("summarize", "Please summarize the content");
  conversation_history.push_back(mojom::ConversationTurn::New(
      "uuid", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "/summarize What is artificial intelligence?", std::nullopt /* prompt */,
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_files */, std::move(smart_mode_entry), false,
      std::nullopt /* model_key */));

  MockConversationAPIClient* mock_client = GetMockConversationAPIClient();

  // Expect that PerformRequest is called with conversation events that include
  // both smart mode definition message and the main user message
  EXPECT_CALL(*mock_client, PerformRequest)
      .WillOnce([&](std::vector<ConversationEvent> conversation,
                    const std::string& selected_language,
                    std::optional<base::Value::List> oai_tool_definitions,
                    const std::optional<std::string>& preferred_tool_name,
                    mojom::ConversationCapability conversation_capability,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Should have 2 events: smart mode definition + main user message
        ASSERT_EQ(conversation.size(), 2u);

        // First event should be the smart mode definition
        EXPECT_EQ(conversation[0].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[0].type, ConversationEventType::kChatMessage);
        EXPECT_EQ(GetContentStrings(conversation[0].content)[0],
                  "When handling the request, interpret '/summarize' as "
                  "'Please summarize the content'");

        // Second event should be the actual user message
        EXPECT_EQ(conversation[1].role, ConversationEventRole::kUser);
        EXPECT_EQ(conversation[1].type, ConversationEventType::kChatMessage);
        EXPECT_EQ(GetContentStrings(conversation[1].content)[0],
                  "/summarize What is artificial intelligence?");

        // Mock successful response
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("AI is a technology..."));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateAssistantResponse(
      {}, conversation_history, "en-US", false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT,
      base::BindRepeating([](EngineConsumer::GenerationResultData) {}),
      future.GetCallback());

  // Wait for the response
  auto result = future.Take();
  EXPECT_TRUE(result.has_value());
}

}  // namespace ai_chat

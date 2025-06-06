/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/base64.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/numerics/clamped_math.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
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

}  // namespace

using ConversationEvent = ConversationAPIClient::ConversationEvent;

class MockConversationAPIClient : public ConversationAPIClient {
 public:
  explicit MockConversationAPIClient(const std::string& model_name)
      : ConversationAPIClient(model_name, nullptr, nullptr, nullptr) {}
  ~MockConversationAPIClient() override = default;

  MOCK_METHOD(void,
              PerformRequest,
              (const std::vector<ConversationEvent>&,
               const std::string& selected_language,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback,
               const std::optional<std::string>& model_name),
              (override));

  std::string GetEventsJson(
      const std::vector<ConversationEvent>& conversation) {
    auto body = CreateJSONRequestBody(conversation, "", std::nullopt, true);
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
        model_service_.get());
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
  std::string page_content(kTestingMaxAssociatedContentLength + 1, 'a');
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Some structured EXPECT calls to catch nicer errors first
        EXPECT_EQ(conversation.size(), 2u);
        EXPECT_EQ(conversation[0].role, mojom::CharacterType::HUMAN);
        // Page content should be truncated
        EXPECT_EQ(conversation[0].content[0], expected_page_content);
        EXPECT_EQ(conversation[0].type, ConversationAPIClient::PageText);
        EXPECT_EQ(conversation[1].role, mojom::CharacterType::HUMAN);
        // Match entire structure
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = "Which show is this about?";
  turn->prompt = "Tell the user which show is this about?";
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      false, page_content, history, "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_WithSelectedText) {
  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": "This is a page about The Mandalorian."},
    {"role": "user", "type": "pageExcerpt", "content": "The Mandalorian"},
    {"role": "user", "type": "chatMessage", "content": "Is this related to a broader series?"}
  ])";
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Some structured EXPECT calls to catch nicer errors first
        EXPECT_EQ(conversation.size(), 3u);
        EXPECT_EQ(conversation[0].role, mojom::CharacterType::HUMAN);
        EXPECT_EQ(conversation[0].type, ConversationAPIClient::PageText);
        EXPECT_EQ(conversation[1].role, mojom::CharacterType::HUMAN);
        EXPECT_EQ(conversation[1].type, ConversationAPIClient::PageExcerpt);
        EXPECT_EQ(conversation[2].role, mojom::CharacterType::HUMAN);
        // Match entire structure
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = "Is this related to a broader series?";
  turn->selected_text = "The Mandalorian";
  history.push_back(std::move(turn));

  engine_->GenerateAssistantResponse(
      false, "This is a page about The Mandalorian.", history, "",
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateEvents_HistoryWithSelectedText) {
  // Tests events building from history with selected text and new query without
  // selected text but with page association.
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Which show is this catchphrase from?", std::nullopt /* prompt */,
      "I have spoken.", std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, false, std::nullopt /* model_key */));
  history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "The Mandalorian.",
      std::nullopt /* prompt */, std::nullopt, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, false, std::nullopt /* model_key */));
  history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::RESPONSE,
      "Is it related to a broader series?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      false, std::nullopt /* model_key */));
  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": "This is my page. I have spoken."},
    {"role": "user", "type": "pageExcerpt", "content": "I have spoken."},
    {"role": "user", "type": "chatMessage", "content": "Which show is this catchphrase from?"},
    {"role": "assistant", "type": "chatMessage", "content": "The Mandalorian."},
    {"role": "user", "type": "chatMessage", "content": "Is it related to a broader series?"}
  ])";
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Some structured EXPECT calls to catch nicer errors first
        EXPECT_EQ(conversation.size(), 5u);
        EXPECT_EQ(conversation[0].role, mojom::CharacterType::HUMAN);
        EXPECT_EQ(conversation[0].type, ConversationAPIClient::PageText);
        EXPECT_EQ(conversation[1].role, mojom::CharacterType::HUMAN);
        EXPECT_EQ(conversation[2].role, mojom::CharacterType::HUMAN);
        EXPECT_EQ(conversation[3].role, mojom::CharacterType::ASSISTANT);
        EXPECT_EQ(conversation[4].role, mojom::CharacterType::HUMAN);
        // Match entire JSON
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });
  engine_->GenerateAssistantResponse(
      false, "This is my page. I have spoken.", history, "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_Rewrite) {
  std::string expected_events = R"([
    {"role": "user", "type": "userText", "content": "Hello World"},
    {"role": "user", "type": "requestRewrite", "content": "Use a funny tone"}
  ])";
  base::RunLoop run_loop;
  auto* mock_api_client = GetMockConversationAPIClient();
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 2u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  engine_->GenerateRewriteSuggestion(
      "Hello World", "Use a funny tone", "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_ModifyReply) {
  // Tests events building from history with modified agent reply.
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Which show is 'This is the way' from?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      false, std::nullopt /* model_key */));

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
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "The Mandalorian.",
      std::nullopt /* prompt */, std::nullopt, std::move(modified_events),
      base::Time::Now(), std::nullopt, std::nullopt, false,
      std::nullopt /* model_key */);
  std::vector<mojom::ConversationTurnPtr> edits;
  edits.push_back(std::move(edit));
  history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "Mandalorian.", std::nullopt /* prompt */,
      std::nullopt, std::move(events), base::Time::Now(), std::move(edits),
      std::nullopt, false, std::nullopt /* model_key */));
  history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Is it related to a broader series?", std::nullopt /* prompt */,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      false, std::nullopt /* model_key */));
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Some structured EXPECT calls to catch nicer errors first
        ASSERT_EQ(conversation.size(), 4u);
        EXPECT_EQ(conversation[0].role, mojom::CharacterType::HUMAN);
        EXPECT_EQ(conversation[0].type, ConversationAPIClient::PageText);
        EXPECT_EQ(conversation[1].role, mojom::CharacterType::HUMAN);
        EXPECT_EQ(conversation[2].role, mojom::CharacterType::ASSISTANT);
        EXPECT_EQ(conversation[3].role, mojom::CharacterType::HUMAN);
        // Match entire JSON
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });
  engine_->GenerateAssistantResponse(
      false, "I have spoken.", history, "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_EarlyReturn) {
  EngineConsumer::ConversationHistory history;
  auto* mock_api_client = GetMockConversationAPIClient();
  auto run_loop = std::make_unique<base::RunLoop>();
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _)).Times(0);
  engine_->GenerateAssistantResponse(
      false, "This is my page.", history, "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult result) {
            run_loop->Quit();
          }));
  run_loop->Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);

  mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::ASSISTANT,
      mojom::ActionType::RESPONSE, "", std::nullopt /* prompt */, std::nullopt,
      std::vector<mojom::ConversationEntryEventPtr>{}, base::Time::Now(),
      std::nullopt, std::nullopt, false, std::nullopt /* model_key */);
  entry->events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("Me")));
  history.push_back(std::move(entry));

  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _)).Times(0);
  run_loop = std::make_unique<base::RunLoop>();
  engine_->GenerateAssistantResponse(
      false, "This is my page.", history, "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult result) {
            run_loop->Quit();
          }));
  run_loop->Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

TEST_F(EngineConsumerConversationAPIUnitTest, GenerateEvents_SummarizePage) {
  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": "This is a sample page content."},
    {"role": "user", "type": "requestSummary", "content": ""}
  ])";
  auto* mock_api_client = GetMockConversationAPIClient();
  base::RunLoop run_loop;
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        // Match entire structure to ensure the generated JSON is correct
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(""));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });
  std::vector<mojom::ConversationTurnPtr> history;
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New();
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->action_type = mojom::ActionType::SUMMARIZE_PAGE;
  turn->text =
      "Summarize the content of this page.";  // This text should be ignored
  history.push_back(std::move(turn));
  engine_->GenerateAssistantResponse(
      false, "This is a sample page content.", history, "", base::DoNothing(),
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        ASSERT_EQ(conversation.size(), 3u);
        EXPECT_EQ(conversation[0].role, mojom::CharacterType::HUMAN);
        for (size_t i = 0; i < 3; ++i) {
          EXPECT_EQ(
              conversation[0].content[i],
              base::StrCat({"data:image/png;base64,",
                            base::Base64Encode(uploaded_images[i]->data)}));
        }
        EXPECT_EQ(conversation[0].type, ConversationAPIClient::UploadImage);
        for (size_t i = 3; i < uploaded_images.size(); ++i) {
          EXPECT_EQ(
              conversation[1].content[i - 3],
              base::StrCat({"data:image/png;base64,",
                            base::Base64Encode(uploaded_images[i]->data)}));
        }
        EXPECT_EQ(conversation[1].type, ConversationAPIClient::PageScreenshot);
        EXPECT_EQ(conversation[2].role, mojom::CharacterType::HUMAN);
        EXPECT_EQ(conversation[2].content[0], kTestPrompt);
        EXPECT_EQ(conversation[2].type, ConversationAPIClient::ChatMessage);
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(kAssistantResponse));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      });

  std::vector<mojom::ConversationTurnPtr> history;
  history.push_back(mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::UNSPECIFIED,
      "What are these images?", kTestPrompt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, Clone(uploaded_images), false,
      std::nullopt /* model_key */));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateAssistantResponse(false, "", history, "", base::DoNothing(),
                                     future.GetCallback());
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(3)
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic3\", \"topic4\", \"topic5\", "
                    "\"topic6\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(2)
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
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

  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(3)
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic3\", \"topic4\", \"topic5\", "
                    "\"topic6\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(3)
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New("not well structured"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(3)
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                    "\"topic7\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events2));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"topics\": [\"topic3\", \"topic4\", \"topic5\", "
                    "\"topic6\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly([&](const std::vector<ConversationEvent>& conversation,
                          const std::string& selected_language,
                          EngineConsumer::GenerationDataCallback data_callback,
                          EngineConsumer::GenerationCompletedCallback callback,
                          const std::optional<std::string>& model_name) {
        ASSERT_EQ(conversation.size(), 1u);
        EXPECT_EQ(conversation[0].type,
                  ConversationAPIClient::GetSuggestedTopicsForFocusTabs);
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(2)
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id1\", \"id2\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
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

  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(2)
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                  FormatComparableEventsJson(expected_events1));
        auto completion_event =
            mojom::ConversationEntryEvent::NewCompletionEvent(
                mojom::CompletionEvent::New(
                    "{ \"tab_ids\": [\"id3\", \"id5\"] }"));
        std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(completion_event), std::nullopt)));
      })
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
                    EngineConsumer::GenerationDataCallback data_callback,
                    EngineConsumer::GenerationCompletedCallback callback,
                    const std::optional<std::string>& model_name) {
        EXPECT_EQ(conversation.size(), 1u);
        EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(2)
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
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
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
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
  EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
      .Times(2)
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
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
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
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
  std::string page_content = "Sample page content.";
  bool is_video = false;
  std::string selected_language = "en-US";

  std::string expected_events = R"([
    {"role": "user", "type": "pageText", "content": "Sample page content."},
    {"role": "user", "type": "requestSuggestedActions", "content": ""}
  ])";

  auto* mock_api_client = GetMockConversationAPIClient();

  // Test successful response
  {
    EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
        .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                      const std::string& language,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          EXPECT_EQ(conversation.size(), 2u);
          EXPECT_EQ(mock_api_client->GetEventsJson(conversation),
                    FormatComparableEventsJson(expected_events));
          auto completion_event =
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New("question1|question2|question3"));
          std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
              std::move(completion_event), std::nullopt)));
        });

    engine_->GenerateQuestionSuggestions(
        is_video, page_content, selected_language,
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
    EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
        .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                      const std::string& language,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          std::move(callback).Run(
              base::unexpected(mojom::APIError::RateLimitReached));
        });

    engine_->GenerateQuestionSuggestions(
        is_video, page_content, selected_language,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_FALSE(result.has_value());
          EXPECT_EQ(result.error(), mojom::APIError::RateLimitReached);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test empty completion event
  {
    EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
        .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                      const std::string& language,
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
        is_video, page_content, selected_language,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_FALSE(result.has_value());
          EXPECT_EQ(result.error(), mojom::APIError::InternalError);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test null event
  {
    EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
        .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                      const std::string& language,
                      EngineConsumer::GenerationDataCallback data_callback,
                      EngineConsumer::GenerationCompletedCallback callback,
                      const std::optional<std::string>& model_name) {
          std::move(callback).Run(base::ok(
              EngineConsumer::GenerationResultData(nullptr, std::nullopt)));
        });

    engine_->GenerateQuestionSuggestions(
        is_video, page_content, selected_language,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_FALSE(result.has_value());
          EXPECT_EQ(result.error(), mojom::APIError::InternalError);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }

  // Test non-completion event
  {
    EXPECT_CALL(*mock_api_client, PerformRequest(_, _, _, _, _))
        .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                      const std::string& language,
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
        is_video, page_content, selected_language,
        base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                      mojom::APIError> result) {
          ASSERT_FALSE(result.has_value());
          EXPECT_EQ(result.error(), mojom::APIError::InternalError);
        }));

    testing::Mock::VerifyAndClearExpectations(mock_api_client);
  }
}

TEST_F(EngineConsumerConversationAPIUnitTest,
       GenerateAssistantResponse_WithModelKeyOverride) {
  auto* mock_api_client = GetMockConversationAPIClient();
  constexpr char kModelKey[] = "chat-basic";

  // Expect PerformRequest with the overridden model name
  EXPECT_CALL(
      *mock_api_client,
      PerformRequest(_, _, _, _,
                     testing::Eq(std::optional<std::string>(
                         model_service_->GetLeoModelNameByKey(kModelKey)))))
      .WillOnce([&](const std::vector<ConversationEvent>& conversation,
                    const std::string& selected_language,
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
  turn->character_type = mojom::CharacterType::HUMAN;
  turn->text = "What is this about?";
  turn->model_key = kModelKey;
  history.push_back(std::move(turn));

  base::RunLoop run_loop;
  engine_->GenerateAssistantResponse(
      false, "This is a test page content.", std::move(history), "",
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_api_client);
}

}  // namespace ai_chat

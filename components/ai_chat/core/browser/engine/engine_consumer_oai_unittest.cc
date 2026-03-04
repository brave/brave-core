/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_oai.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/json/string_escape.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/test_utils.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using base::test::ParseJsonDict;
using ::testing::_;
using ::testing::Sequence;

namespace ai_chat {

namespace {

struct GenerateRewriteTestParam {
  std::string name;
  mojom::ActionType action_type;
  mojom::ContentBlock::Tag expected_content_type;
  int message_id;
  std::optional<std::string> tone;
  std::optional<mojom::SimpleRequestType> expected_simple_request_type;
};

}  // namespace

class MockCallback {
 public:
  MOCK_METHOD(void, OnDataReceived, (mojom::ConversationEntryEventPtr));
  MOCK_METHOD(void, OnCompleted, (EngineConsumer::GenerationResult));
};

class MockOAIAPIClient : public OAIAPIClient {
 public:
  MockOAIAPIClient() : OAIAPIClient(nullptr) {}
  ~MockOAIAPIClient() override = default;

  MOCK_METHOD(void,
              PerformRequest,
              (const mojom::CustomModelOptions&,
               std::vector<OAIMessage>,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback,
               const std::optional<std::vector<std::string>>&),
              (override));

  std::string GetMessagesJson(std::vector<OAIMessage> messages) {
    auto serialized = OAIAPIClient::SerializeOAIMessages(std::move(messages));
    std::string messages_json;
    base::JSONWriter::WriteWithOptions(
        serialized, base::JSONWriter::OPTIONS_PRETTY_PRINT, &messages_json);
    return messages_json;
  }
};

class EngineConsumerOAIUnitTest : public testing::Test {
 public:
  EngineConsumerOAIUnitTest() = default;
  ~EngineConsumerOAIUnitTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(pref_service_.registry());

    auto options = mojom::CustomModelOptions::New();
    options->endpoint = GURL("https://test.com/");
    options->model_request_name = "request_name";
    options->context_size = 5000;
    options->max_associated_content_length = 17200;
    options->model_system_prompt = "This is a custom system prompt.";
    options->api_key = "api_key";

    model_ = mojom::Model::New();
    model_->key = "test_model_key";
    model_->display_name = "Test Model Display Name";
    model_->options =
        mojom::ModelOptions::NewCustomModelOptions(std::move(options));

    engine_ = std::make_unique<EngineConsumerOAIRemote>(
        *model_->options->get_custom_model_options(), nullptr, nullptr,
        &pref_service_);

    engine_->SetAPIForTesting(std::make_unique<MockOAIAPIClient>());
  }

  MockOAIAPIClient* GetClient() {
    return static_cast<MockOAIAPIClient*>(engine_->GetAPIForTesting());
  }

  std::string FormatComparableMessagesJson(std::string_view formatted_json) {
    auto messages = base::test::ParseJson(formatted_json);
    std::string messages_json;
    base::JSONWriter::WriteWithOptions(
        messages, base::JSONWriter::OPTIONS_PRETTY_PRINT, &messages_json);
    return messages_json;
  }

  void TearDown() override {}

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple pref_service_;
  mojom::ModelPtr model_;
  std::unique_ptr<EngineConsumerOAIRemote> engine_;
};

TEST_F(EngineConsumerOAIUnitTest, UpdateModelOptions) {
  PageContent page_content("Page content", false);
  auto* client = GetClient();

  base::RunLoop run_loop;
  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce([&run_loop, this](
                    const mojom::CustomModelOptions& model_options,
                    std::vector<OAIMessage>,
                    EngineConsumer::GenerationDataCallback,
                    EngineConsumer::GenerationCompletedCallback,
                    const std::optional<std::vector<std::string>>&) {
        EXPECT_EQ("https://test.com/", model_options.endpoint.spec());

        // Update the model options
        auto options = mojom::CustomModelOptions::New();
        options->endpoint = GURL("https://updated-test.com");
        options->model_request_name = "request_name";
        options->model_system_prompt = "";
        options->api_key = "api_key";

        model_ = mojom::Model::New();
        model_->key = "test_model_key";
        model_->display_name = "Test Model Display Name";
        model_->options =
            mojom::ModelOptions::NewCustomModelOptions(std::move(options));
        engine_->UpdateModelOptions(*model_->options);

        run_loop.Quit();
      });

  engine_->GenerateQuestionSuggestions({page_content}, base::NullCallback());
  run_loop.Run();

  base::RunLoop run_loop2;
  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce([&run_loop2](const mojom::CustomModelOptions& model_options,
                             std::vector<OAIMessage>,
                             EngineConsumer::GenerationDataCallback,
                             EngineConsumer::GenerationCompletedCallback,
                             const std::optional<std::vector<std::string>>&) {
        EXPECT_EQ("https://updated-test.com/", model_options.endpoint.spec());
        run_loop2.Quit();
      });

  engine_->GenerateQuestionSuggestions({page_content}, base::NullCallback());
  run_loop2.Run();

  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, GenerateQuestionSuggestions) {
  PageContent page_content("This is a test page content", false);

  auto* client = GetClient();
  base::RunLoop run_loop;

  auto invoke_completion_callback = [](const std::string& result_string) {
    return [result_string](
               const mojom::CustomModelOptions&, std::vector<OAIMessage>,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback completed_callback,
               const std::optional<std::vector<std::string>>&) {
      std::move(completed_callback)
          .Run(base::ok(EngineConsumer::GenerationResultData(
              mojom::ConversationEntryEvent::NewCompletionEvent(
                  mojom::CompletionEvent::New(result_string)),
              std::nullopt /* model_key */)));
    };
  };

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(invoke_completion_callback("Returning non question format"))
      .WillOnce(invoke_completion_callback(
          "<question>Question 1</question><question>Question 2</question>"))
      .WillOnce(invoke_completion_callback(
          "<question>Question 1</question>\n\n<question>Question 2</question>"))
      .WillOnce(invoke_completion_callback(
          "<question>Question 1</question><question>Question 2</question>"
          "<question>Question 3</question>"));

  engine_->GenerateQuestionSuggestions(
      {page_content}, base::BindLambdaForTesting(
                          [](EngineConsumer::SuggestedQuestionResult result) {
                            EXPECT_TRUE(result.has_value());
                            EXPECT_EQ(result.value().size(), 1ull);
                          }));

  engine_->GenerateQuestionSuggestions(
      {page_content}, base::BindLambdaForTesting(
                          [](EngineConsumer::SuggestedQuestionResult result) {
                            EXPECT_EQ(result.value()[0], "Question 1");
                            EXPECT_EQ(result.value()[1], "Question 2");
                          }));

  engine_->GenerateQuestionSuggestions(
      {page_content}, base::BindLambdaForTesting(
                          [](EngineConsumer::SuggestedQuestionResult result) {
                            EXPECT_EQ(result.value()[0], "Question 1");
                            EXPECT_EQ(result.value()[1], "Question 2");
                          }));

  engine_->GenerateQuestionSuggestions(
      {page_content},
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::SuggestedQuestionResult result) {
            ASSERT_TRUE(result.has_value());
            ASSERT_EQ(result->size(), 3u);
            EXPECT_EQ((*result)[0], "Question 1");
            EXPECT_EQ((*result)[1], "Question 2");
            EXPECT_EQ((*result)[2], "Question 3");
            run_loop.Quit();
          }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, GenerateQuestionSuggestions_Errors) {
  PageContent page_content("This is a test page content", false);
  auto* client = GetClient();

  // Test error case: result doesn't have a value
  {
    base::RunLoop run_loop;
    EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
        .WillOnce(
            [](const mojom::CustomModelOptions&, std::vector<OAIMessage>,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback completed_callback,
               const std::optional<std::vector<std::string>>&) {
              // Return an error response (result without a value)
              std::move(completed_callback)
                  .Run(base::unexpected(mojom::APIError::RateLimitReached));
            });

    engine_->GenerateQuestionSuggestions(
        {page_content},
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::SuggestedQuestionResult result) {
              // Check that error is properly propagated
              EXPECT_FALSE(result.has_value());
              EXPECT_EQ(result.error(), mojom::APIError::RateLimitReached);
              run_loop.Quit();
            }));

    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(client);
  }

  // Test error case: result has an empty event
  {
    base::RunLoop run_loop;
    EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
        .WillOnce(
            [](const mojom::CustomModelOptions&, std::vector<OAIMessage>,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback completed_callback,
               const std::optional<std::vector<std::string>>&) {
              // Return a result with a null event
              std::move(completed_callback)
                  .Run(base::ok(EngineConsumer::GenerationResultData(
                      nullptr, std::nullopt /* model_key */)));
            });

    engine_->GenerateQuestionSuggestions(
        {page_content},
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::SuggestedQuestionResult result) {
              // Check that error is properly propagated
              EXPECT_FALSE(result.has_value());
              EXPECT_EQ(result.error(), mojom::APIError::InternalError);
              run_loop.Quit();
            }));

    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(client);
  }

  // Test error case: result has a non-completion event
  {
    base::RunLoop run_loop;
    EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
        .WillOnce(
            [](const mojom::CustomModelOptions&, std::vector<OAIMessage>,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback completed_callback,
               const std::optional<std::vector<std::string>>&) {
              // Return a result with a non-completion event
              std::move(completed_callback)
                  .Run(base::ok(EngineConsumer::GenerationResultData(
                      mojom::ConversationEntryEvent::NewSearchStatusEvent(
                          mojom::SearchStatusEvent::New(true)),
                      std::nullopt /* model_key */)));
            });

    engine_->GenerateQuestionSuggestions(
        {page_content},
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::SuggestedQuestionResult result) {
              // Check that error is properly propagated
              EXPECT_FALSE(result.has_value());
              EXPECT_EQ(result.error(), mojom::APIError::InternalError);
              run_loop.Quit();
            }));

    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(client);
  }

  // Test error case: result has an empty completion
  {
    base::RunLoop run_loop;
    EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
        .WillOnce(
            [](const mojom::CustomModelOptions&, std::vector<OAIMessage>,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback completed_callback,
               const std::optional<std::vector<std::string>>&) {
              // Return a result with an empty completion
              std::move(completed_callback)
                  .Run(base::ok(EngineConsumer::GenerationResultData(
                      mojom::ConversationEntryEvent::NewCompletionEvent(
                          mojom::CompletionEvent::New("")),
                      std::nullopt /* model_key */)));
            });

    engine_->GenerateQuestionSuggestions(
        {page_content},
        base::BindLambdaForTesting(
            [&run_loop](EngineConsumer::SuggestedQuestionResult result) {
              // Check that error is properly propagated
              EXPECT_FALSE(result.has_value());
              EXPECT_EQ(result.error(), mojom::APIError::InternalError);
              run_loop.Quit();
            }));

    run_loop.Run();
    testing::Mock::VerifyAndClearExpectations(client);
  }
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateAssistantResponseWithDefaultSystemPrompt) {
  PageContent page_content("Page content 1", false);
  // Create a set of options WITHOUT a custom system prompt.
  auto options = mojom::CustomModelOptions::New();
  options->endpoint = GURL("https://test.com/");
  options->model_request_name = "request_name";
  options->api_key = "api_key";
  options->max_associated_content_length = 1000;

  // Build a new model with the prompt-less options.
  model_ = mojom::Model::New();
  model_->key = "test_model_key";
  model_->display_name = "Test Model Display Name";
  model_->options =
      mojom::ModelOptions::NewCustomModelOptions(std::move(options));

  // Create a new engine with the new model.
  engine_ = std::make_unique<EngineConsumerOAIRemote>(
      *model_->options->get_custom_model_options(), nullptr, nullptr,
      &pref_service_);
  engine_->SetAPIForTesting(std::make_unique<MockOAIAPIClient>());

  EngineConsumer::ConversationHistory history;

  // Critical strings for the test.
  std::string human_input = "Hello, how are you?";
  std::string assistant_response = "I'm fine, thank you.";

  std::string date_and_time_string =
      base::UTF16ToUTF8(TimeFormatFriendlyDateAndTime(base::Time::Now()));

  std::string expected_system_message = base::ReplaceStringPlaceholders(
      l10n_util::GetStringUTF8(IDS_AI_CHAT_DEFAULT_CUSTOM_MODEL_SYSTEM_PROMPT),
      {date_and_time_string}, nullptr);

  // Push a single user turn into the history.
  history.push_back(mojom::ConversationTurn::New(
      "turn-1",
      mojom::CharacterType::HUMAN,     // Author is the user
      mojom::ActionType::UNSPECIFIED,  // No specific action
      human_input,                     // User message
      std::nullopt,                    // No prompt
      std::nullopt,                    // No selected text
      std::nullopt,                    // No events
      base::Time::Now(),               // Current time
      std::nullopt,                    // No message edits
      std::nullopt,                    // No uploaded images
      nullptr,                         // No skill
      false,                           // Not from Brave SERP
      std::nullopt,                    // No model_key
      nullptr                          // near_verification_status
      ));

  // Prepare to capture API client request
  auto* client = GetClient();
  auto run_loop = std::make_unique<base::RunLoop>();

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [&expected_system_message, &human_input, &assistant_response](
              const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            ASSERT_EQ(messages.size(), 2u);

            // System message
            EXPECT_EQ(messages[0].role, "system");
            ASSERT_EQ(messages[0].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[0].content[0],
                            expected_system_message);

            // User message with page content + user text
            EXPECT_EQ(messages[1].role, "user");
            ASSERT_EQ(messages[1].content.size(), 2u);
            VerifyPageTextBlock(FROM_HERE, messages[1].content[0],
                                "Page content 1");
            VerifyTextBlock(FROM_HERE, messages[1].content.back(), human_input);

            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(assistant_response)),
                    std::nullopt /* model_key */)));
          });

  // Initiate the test
  engine_->GenerateAssistantResponse(
      {{{"turn-1", {page_content}}}}, history, false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting([&run_loop, &assistant_response](
                                     EngineConsumer::GenerationResult result) {
        EXPECT_EQ(result.value(),
                  EngineConsumer::GenerationResultData(
                      mojom::ConversationEntryEvent::NewCompletionEvent(
                          mojom::CompletionEvent::New(assistant_response)),
                      std::nullopt /* model_key */));
        run_loop->Quit();
      }));

  // Run the test
  run_loop->Run();

  // Verify the expectations
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateAssistantResponseWithCustomSystemPrompt) {
  EngineConsumer::ConversationHistory history;

  std::string human_input = "Which show is this catchphrase from?";
  std::string selected_text = "This is the way.";
  std::string assistant_input = "This is mandalorian.";
  std::string expected_system_message = "This is a custom system prompt.";

  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN,
      mojom::ActionType::SUMMARIZE_SELECTED_TEXT, human_input,
      std::nullopt /* prompt */, selected_text, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt /* model_key */, nullptr /* near_verification_status */));

  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      assistant_input, std::nullopt /* prompt */, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt /* model_key */, nullptr /* near_verification_status */));

  auto* client = GetClient();
  auto run_loop = std::make_unique<base::RunLoop>();

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [&expected_system_message, &assistant_input, &human_input](
              const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            // system + human turn 1 + assistant turn + human turn 3
            ASSERT_EQ(messages.size(), 4u);

            // System message
            EXPECT_EQ(messages[0].role, "system");
            ASSERT_EQ(messages[0].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[0].content[0],
                            expected_system_message);

            // First human turn with selected text + user text
            EXPECT_EQ(messages[1].role, "user");
            ASSERT_EQ(messages[1].content.size(), 2u);
            VerifyPageExcerptBlock(FROM_HERE, messages[1].content[0],
                                   "This is the way.");
            VerifyTextBlock(FROM_HERE, messages[1].content[1], human_input);

            // Assistant turn
            EXPECT_EQ(messages[2].role, "assistant");
            ASSERT_EQ(messages[2].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[2].content[0], assistant_input);

            // Third human turn
            EXPECT_EQ(messages[3].role, "user");
            ASSERT_EQ(messages[3].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[3].content.back(),
                            "What's his name?");

            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("I dont know")),
                    std::nullopt /* model_key */)));
          });

  {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New();
    entry->uuid = "turn-3";
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->text = "What's his name?";
    history.push_back(std::move(entry));
  }

  engine_->GenerateAssistantResponse(
      {}, history, false, {}, std::nullopt, mojom::ConversationCapability::CHAT,
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult result) {
            EXPECT_EQ(result.value(),
                      EngineConsumer::GenerationResultData(
                          mojom::ConversationEntryEvent::NewCompletionEvent(
                              mojom::CompletionEvent::New("I dont know")),
                          std::nullopt /* model_key */));
            run_loop->Quit();
          }));

  run_loop->Run();
  testing::Mock::VerifyAndClearExpectations(client);

  // Test with a modified server reply.
  run_loop = std::make_unique<base::RunLoop>();
  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [&expected_system_message](
              const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            // system + human turn 1 + assistant turn + human turn 3
            ASSERT_EQ(messages.size(), 4u);

            EXPECT_EQ(messages[0].role, "system");
            ASSERT_EQ(messages[0].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[0].content[0],
                            expected_system_message);

            EXPECT_EQ(messages[1].role, "user");
            ASSERT_EQ(messages[1].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[1].content.back(),
                            "Which show is 'This is the way' from?");

            // Modified server reply should be used here.
            EXPECT_EQ(messages[2].role, "assistant");
            ASSERT_EQ(messages[2].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[2].content[0],
                            "The Mandalorian.");

            EXPECT_EQ(messages[3].role, "user");
            ASSERT_EQ(messages[3].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[3].content.back(),
                            "Is it related to a broader series?");

            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("")),
                    std::nullopt /* model_key */)));
          });

  engine_->GenerateAssistantResponse(
      {}, GetHistoryWithModifiedReply(), false, {}, std::nullopt,
      mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult result) {
            run_loop->Quit();
          }));
  run_loop->Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, ShouldCallSanitizeInputOnPageContent) {
  class MockOAIEngineConsumer : public EngineConsumerOAIRemote {
   public:
    using EngineConsumerOAIRemote::EngineConsumerOAIRemote;
    ~MockOAIEngineConsumer() override = default;
    MOCK_METHOD(void, SanitizeInput, (std::string & input), (override));
  };

  PageContent page_content_1("This is a page about The Mandalorian.", false);
  PageContent page_content_2("This is a video about The Mandalorian.", true);

  auto mock_engine_consumer = std::make_unique<MockOAIEngineConsumer>(
      *model_->options->get_custom_model_options(), nullptr, nullptr,
      &pref_service_);
  mock_engine_consumer->SetAPIForTesting(std::make_unique<MockOAIAPIClient>());

  // Calling GenerateAssistantResponse should call SanitizeInput
  {
    EXPECT_CALL(*mock_engine_consumer, SanitizeInput(page_content_1.content));
    EXPECT_CALL(*mock_engine_consumer, SanitizeInput(page_content_2.content));

    std::vector<mojom::ConversationTurnPtr> history;
    auto turn = mojom::ConversationTurn::New();
    turn->uuid = "turn-1";
    history.push_back(std::move(turn));
    mock_engine_consumer->GenerateAssistantResponse(
        {{{history.back()->uuid.value(), {page_content_1, page_content_2}}}},
        history, false, {}, std::nullopt, mojom::ConversationCapability::CHAT,
        base::DoNothing(), base::DoNothing());
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

TEST_F(EngineConsumerOAIUnitTest,
       GenerateAssistantResponse_CustomSystemPrompt_NoMemoriesAdded) {
  // Test if user has set a custom system prompt, we won't have user memory
  // instruction added to the system prompt. The system prompt should be the
  // custom system prompt. The user memory instruction should be empty. Should
  // not have user memory message in the request.
  auto* client = GetClient();

  // Setup user customizations and memories
  auto customizations = mojom::Customizations::New();
  customizations->name = "John Doe";
  prefs::SetCustomizationsToPrefs(std::move(customizations), pref_service_);
  prefs::AddMemoryToPrefs("I like to eat apple", pref_service_);

  // Setup conversation history
  EngineConsumer::ConversationHistory history;
  mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New();
  entry->uuid = "turn-1";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->text = "What is my name?";
  history.push_back(std::move(entry));

  base::RunLoop run_loop;
  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>&) {
            ASSERT_EQ(messages.size(), 2u);
            EXPECT_EQ(messages[0].role, "system");
            ASSERT_EQ(messages[0].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[0].content[0],
                            "This is a custom system prompt.");
            EXPECT_EQ(messages[1].role, "user");
            ASSERT_EQ(messages[1].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[1].content.back(),
                            "What is my name?");

            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("")),
                    std::nullopt /* model_key */)));
          });

  engine_->GenerateAssistantResponse(
      {}, history, false, {}, std::nullopt, mojom::ConversationCapability::CHAT,
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));

  run_loop.Run();
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateAssistantResponse_DefaultSystemPrompt_MemoriesAdded) {
  // Test if user has not set a custom system prompt, we will have user memory
  // instruction added to the system prompt. The system prompt should be the
  // default system prompt. The user memory instruction should be added to the
  // system prompt. The user memory should be added to the request.

  // Setup the model options and update the model options via the engine.
  auto options = mojom::CustomModelOptions::New();
  options->endpoint = GURL("https://test.com/");
  options->model_request_name = "request_name";
  options->context_size = 5000;
  options->max_associated_content_length = 17200;
  options->model_system_prompt = std::nullopt;
  model_->options =
      mojom::ModelOptions::NewCustomModelOptions(std::move(options));
  engine_->UpdateModelOptions(*model_->options);

  auto* client = GetClient();

  // Setup the history
  EngineConsumer::ConversationHistory history;
  mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New();
  entry->uuid = "turn-1";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->text = "What is my name?";
  history.push_back(std::move(entry));

  // Setup user customizations and memories
  auto customizations = mojom::Customizations::New();
  customizations->name = "John Doe";
  customizations->other = "<user_memory>tag</user_memory>";
  prefs::SetCustomizationsToPrefs(std::move(customizations), pref_service_);
  prefs::AddMemoryToPrefs("I like to eat apple", pref_service_);
  prefs::AddMemoryToPrefs("<script>alert('xss')</script>", pref_service_);

  // Setup the expected system message
  std::string date_and_time_string =
      base::UTF16ToUTF8(TimeFormatFriendlyDateAndTime(base::Time::Now()));
  std::string expected_system_message = base::ReplaceStringPlaceholders(
      l10n_util::GetStringUTF8(IDS_AI_CHAT_DEFAULT_CUSTOM_MODEL_SYSTEM_PROMPT),
      {date_and_time_string}, nullptr);
  base::StrAppend(
      &expected_system_message,
      {l10n_util::GetStringUTF8(
          IDS_AI_CHAT_CUSTOM_MODEL_USER_MEMORY_SYSTEM_PROMPT_SEGMENT)});

  base::RunLoop run_loop;
  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [&](const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            ASSERT_EQ(messages.size(), 2u);

            // System message should include memory system prompt segment
            EXPECT_EQ(messages[0].role, "system");
            ASSERT_EQ(messages[0].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[0].content[0],
                            expected_system_message);

            // User message should have memory block and user text
            EXPECT_EQ(messages[1].role, "user");
            ASSERT_EQ(messages[1].content.size(), 2u);
            VerifyMemoryBlock(
                FROM_HERE, messages[1].content[0],
                BuildExpectedMemory(
                    {{"name", "John Doe"},
                     {"other", "&lt;user_memory&gt;tag&lt;/user_memory&gt;"}},
                    {{"memories",
                      {"I like to eat apple",
                       "&lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt;"}}}));
            VerifyTextBlock(FROM_HERE, messages[1].content[1],
                            "What is my name?");

            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("")),
                    std::nullopt /* model_key */)));
          });

  engine_->GenerateAssistantResponse(
      {}, history, false, {}, std::nullopt, mojom::ConversationCapability::CHAT,
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));

  run_loop.Run();
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateAssistantResponse_TemporaryChatExcludesMemory) {
  // Setup the model options to not have a custom system prompt
  auto options = mojom::CustomModelOptions::New();
  options->endpoint = GURL("https://test.com/");
  options->model_request_name = "request_name";
  options->api_key = "api_key";
  options->model_system_prompt = std::nullopt;

  model_ = mojom::Model::New();
  model_->key = "test_model_key";
  model_->display_name = "Test Model Display Name";
  model_->options =
      mojom::ModelOptions::NewCustomModelOptions(std::move(options));

  engine_ = std::make_unique<EngineConsumerOAIRemote>(
      *model_->options->get_custom_model_options(), nullptr, nullptr,
      &pref_service_);
  engine_->SetAPIForTesting(std::make_unique<MockOAIAPIClient>());

  // Setup user memory to ensure it's available but should be excluded
  auto customizations = mojom::Customizations::New();
  customizations->name = "John Doe";
  customizations->other = "Software Engineer";
  prefs::SetCustomizationsToPrefs(std::move(customizations), pref_service_);
  prefs::AddMemoryToPrefs("I like to eat apple", pref_service_);

  EngineConsumer::ConversationHistory history;
  mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New();
  entry->uuid = "turn-1";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->text = "What is my name?";
  history.push_back(std::move(entry));

  auto* client = GetClient();
  base::RunLoop run_loop;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [&](const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            // Should only have 2 messages: system prompt and user message
            // NO memory content block should be present
            ASSERT_EQ(messages.size(), 2u);
            EXPECT_EQ(messages[0].role, "system");
            // The system message should contain the default system prompt
            // but NOT include user memory instruction segment
            ASSERT_EQ(messages[0].content.size(), 1u);
            ASSERT_TRUE(messages[0].content[0]->is_text_content_block());
            EXPECT_THAT(messages[0].content[0]->get_text_content_block()->text,
                        testing::Not(testing::HasSubstr("memory")));

            // No memory content block
            EXPECT_EQ(messages[1].role, "user");
            ASSERT_EQ(messages[1].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[1].content.back(),
                            "What is my name?");

            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("")),
                    std::nullopt /* model_key */)));
          });

  engine_->GenerateAssistantResponse(
      {}, history,
      true,  // is_temporary_chat = true
      {}, std::nullopt, mojom::ConversationCapability::CHAT, base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, GenerateConversationTitle_Success) {
  auto* client = GetClient();
  PageContent page_content("This is a test page about AI", false);
  PageContentsMap page_contents;
  page_contents["turn-1"] = {std::cref(page_content)};

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "What is artificial intelligence?", std::nullopt, std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt,
      nullptr /* near_verification_status */));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "AI is a technology that enables machines to simulate human "
      "intelligence.",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt, nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>& stop_sequences) {
            // Verify the stop sequences
            ASSERT_TRUE(stop_sequences.has_value());
            ASSERT_EQ(stop_sequences->size(), 1u);
            EXPECT_EQ((*stop_sequences)[0], "</title>");

            // Verify messages structure - expect 2 messages:
            // 1. user message with page content + title request
            // 2. assistant seed message
            ASSERT_EQ(messages.size(), 2u);

            // First message: user role with content blocks
            EXPECT_EQ(messages[0].role, "user");
            // Should have page content + request title content blocks
            ASSERT_EQ(messages[0].content.size(), 2u);

            // Verify page text content block
            VerifyPageTextBlock(FROM_HERE, messages[0].content[0],
                                "This is a test page about AI");

            // Verify request title content block
            VerifyRequestTitleBlock(FROM_HERE, messages[0].content[1],
                                    "What is artificial intelligence?");

            // Assistant seed message
            EXPECT_EQ(messages[1].role, "assistant");
            ASSERT_EQ(messages[1].content.size(), 1u);
            VerifyTextBlock(FROM_HERE, messages[1].content[0],
                            "Here is the title for the above conversation "
                            "in <title> tags:\n<title>");

            // Simulate successful title generation
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
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

  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateConversationTitle_MaxPerContentLengthRespected) {
  auto* client = GetClient();

  // Content 1: Exactly limit-1 chars (1199) - should NOT be truncated
  std::string content_1199(kMaxContextCharsForTitleGeneration - 1, 'a');
  PageContent page_content1(content_1199, false);

  // Content 2: Exactly limit chars (1200) - should NOT be truncated
  std::string content_1200(kMaxContextCharsForTitleGeneration, 'b');
  PageContent page_content2(content_1200, false);

  // Content 3: Exactly limit+1 chars (1201) - should be truncated to limit
  std::string content_1201(kMaxContextCharsForTitleGeneration + 1, 'c');
  PageContent page_content3(content_1201, true);  // video content

  PageContentsMap page_contents;
  page_contents["turn-1"] = {std::cref(page_content1), std::cref(page_content2),
                             std::cref(page_content3)};

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Analyze these pages", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt, nullptr));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Analysis completed.", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt, nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>&) {
            // Should have 2 messages: user message + assistant seed
            ASSERT_EQ(messages.size(), 2u);

            // First message: user role with content blocks
            // (3 page contents + 1 title request)
            EXPECT_EQ(messages[0].role, "user");
            ASSERT_EQ(messages[0].content.size(), 4u);

            // Content is processed in REVERSE order
            // So: content3 (video, 1201->1200), content2 (page, 1200), content1
            // (page, 1199), + title request

            // First block: content3 (video, 1201 chars) - should be truncated
            // to 1200 (raw content, no wrapper text)
            VerifyVideoTranscriptBlock(
                FROM_HERE, messages[0].content[0],
                std::string(kMaxContextCharsForTitleGeneration, 'c'));

            // Second block: content2 (page, 1200 chars) - should NOT be
            // truncated (raw content, no wrapper text)
            VerifyPageTextBlock(
                FROM_HERE, messages[0].content[1],
                std::string(kMaxContextCharsForTitleGeneration, 'b'));

            // Third block: content1 (page, 1199 chars) - should NOT be
            // truncated (raw content, no wrapper text)
            VerifyPageTextBlock(
                FROM_HERE, messages[0].content[2],
                std::string(kMaxContextCharsForTitleGeneration - 1, 'a'));

            // Fourth block: title request (raw conversation text)
            VerifyRequestTitleBlock(FROM_HERE, messages[0].content[3],
                                    "Analyze these pages");

            // Assistant seed message
            EXPECT_EQ(messages[1].role, "assistant");

            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Content Analysis")),
                    std::nullopt)));
          });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->event->is_conversation_title_event());
  EXPECT_EQ(result->event->get_conversation_title_event()->title,
            "Content Analysis");
}

TEST_F(EngineConsumerOAIUnitTest, GenerateConversationTitle_WithSelectedText) {
  auto* client = GetClient();
  PageContentsMap page_contents;

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Explain this concept", std::nullopt,
      "Machine learning is a subset of AI", std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
      nullptr));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Machine learning allows computers to learn patterns from data.",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt, nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce([](const mojom::CustomModelOptions&,
                   std::vector<OAIMessage> messages,
                   EngineConsumer::GenerationDataCallback,
                   EngineConsumer::GenerationCompletedCallback
                       completed_callback,
                   const std::optional<std::vector<std::string>>&) {
        // Should have 2 messages: user message + assistant seed
        ASSERT_EQ(messages.size(), 2u);

        // First message: user role with content blocks
        // (page excerpt + title request)
        EXPECT_EQ(messages[0].role, "user");
        ASSERT_EQ(messages[0].content.size(), 2u);

        // First block: page excerpt with selected text
        VerifyPageExcerptBlock(FROM_HERE, messages[0].content[0],
                               "Machine learning is a subset of AI");

        // Second block: title request
        VerifyRequestTitleBlock(FROM_HERE, messages[0].content[1],
                                "Explain this concept");

        // Assistant seed message
        EXPECT_EQ(messages[1].role, "assistant");

        std::move(completed_callback)
            .Run(base::ok(EngineConsumer::GenerationResultData(
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New("Machine Learning Explained")),
                std::nullopt)));
      });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->event->is_conversation_title_event());
  EXPECT_EQ(result->event->get_conversation_title_event()->title,
            "Machine Learning Explained");
}

TEST_F(EngineConsumerOAIUnitTest, GenerateConversationTitle_WithUploadedFiles) {
  auto* client = GetClient();
  PageContentsMap page_contents;

  auto uploaded_files =
      CreateSampleUploadedFiles(1, mojom::UploadedFileType::kImage);

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "What's in this image?", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::move(uploaded_files),
      nullptr /* skill */, false, std::nullopt,
      nullptr /* near_verification_status */));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "The image shows a beautiful sunset over mountains.", std::nullopt,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, std::nullopt,
      nullptr /* skill */, false, std::nullopt,
      nullptr /* near_verification_status */));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>&) {
            // Should have 2 messages: user message + assistant seed
            ASSERT_EQ(messages.size(), 2u);

            // First message: user role with title request block
            EXPECT_EQ(messages[0].role, "user");
            ASSERT_EQ(messages[0].content.size(), 1u);

            // When uploaded files are present, title request uses assistant
            // response
            VerifyRequestTitleBlock(
                FROM_HERE, messages[0].content[0],
                "The image shows a beautiful sunset over mountains.");

            // Assistant seed message
            EXPECT_EQ(messages[1].role, "assistant");

            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New("Sunset Mountain View")),
                    std::nullopt)));
          });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->event->is_conversation_title_event());
  EXPECT_EQ(result->event->get_conversation_title_event()->title,
            "Sunset Mountain View");
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateConversationTitle_InvalidHistory_WrongSize) {
  PageContentsMap page_contents;

  // Test with empty history
  {
    EngineConsumer::ConversationHistory history;
    base::test::TestFuture<EngineConsumer::GenerationResult> future;

    engine_->GenerateConversationTitle(page_contents, history,
                                       future.GetCallback());

    auto result = future.Take();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), mojom::APIError::InternalError);
  }

  // Test with single turn
  {
    EngineConsumer::ConversationHistory history;
    history.push_back(mojom::ConversationTurn::New(
        "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
        "Hello", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
        std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
        nullptr));

    base::test::TestFuture<EngineConsumer::GenerationResult> future;
    engine_->GenerateConversationTitle(page_contents, history,
                                       future.GetCallback());

    auto result = future.Take();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), mojom::APIError::InternalError);
  }

  // Test with three turns
  {
    EngineConsumer::ConversationHistory history;
    history.push_back(mojom::ConversationTurn::New(
        "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
        "Hello", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
        std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
        nullptr));
    history.push_back(mojom::ConversationTurn::New(
        "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        "Hi there!", std::nullopt, std::nullopt, std::nullopt,
        base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */,
        false, std::nullopt, nullptr));
    history.push_back(mojom::ConversationTurn::New(
        "turn-3", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
        "How are you?", std::nullopt, std::nullopt, std::nullopt,
        base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */,
        false, std::nullopt, nullptr));

    base::test::TestFuture<EngineConsumer::GenerationResult> future;
    engine_->GenerateConversationTitle(page_contents, history,
                                       future.GetCallback());

    auto result = future.Take();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), mojom::APIError::InternalError);
  }
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateConversationTitle_InvalidHistory_WrongCharacterTypes) {
  PageContentsMap page_contents;

  // Test with two human turns
  {
    EngineConsumer::ConversationHistory history;
    history.push_back(mojom::ConversationTurn::New(
        "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
        "Hello", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
        std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
        nullptr));
    history.push_back(mojom::ConversationTurn::New(
        "turn-2", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
        "How are you?", std::nullopt, std::nullopt, std::nullopt,
        base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */,
        false, std::nullopt, nullptr));

    base::test::TestFuture<EngineConsumer::GenerationResult> future;
    engine_->GenerateConversationTitle(page_contents, history,
                                       future.GetCallback());

    auto result = future.Take();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), mojom::APIError::InternalError);
  }

  // Test with assistant first, then human
  {
    EngineConsumer::ConversationHistory history;
    history.push_back(mojom::ConversationTurn::New(
        "turn-1", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        "Hello", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
        std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
        nullptr));
    history.push_back(mojom::ConversationTurn::New(
        "turn-2", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
        "Hi there!", std::nullopt, std::nullopt, std::nullopt,
        base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */,
        false, std::nullopt, nullptr));

    base::test::TestFuture<EngineConsumer::GenerationResult> future;
    engine_->GenerateConversationTitle(page_contents, history,
                                       future.GetCallback());

    auto result = future.Take();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), mojom::APIError::InternalError);
  }
}

TEST_F(EngineConsumerOAIUnitTest, GenerateConversationTitle_APIError) {
  auto* client = GetClient();
  PageContentsMap page_contents;

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "Hello",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt, nullptr));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Hi there!", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
      nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>&) {
            std::move(completed_callback)
                .Run(base::unexpected(mojom::APIError::RateLimitReached));
          });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);
}

TEST_F(EngineConsumerOAIUnitTest, GenerateConversationTitle_TitleTooLong) {
  auto* client = GetClient();
  PageContentsMap page_contents;

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "Hello",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt, nullptr));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Hi there!", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
      nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>&) {
            std::string long_title(kMaxTitleLength + 1, 'x');
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
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

TEST_F(EngineConsumerOAIUnitTest, GenerateConversationTitle_EmptyResponse) {
  auto* client = GetClient();
  PageContentsMap page_contents;

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "Hello",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt, nullptr));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Hi there!", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
      nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>&) {
            // Return empty completion
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
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

TEST_F(EngineConsumerOAIUnitTest,
       GenerateConversationTitle_WhitespaceHandling) {
  auto* client = GetClient();
  PageContentsMap page_contents;

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "Hello",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt, nullptr));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Hi there!", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
      nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>&) {
            // Return title with leading/trailing whitespace
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(
                            "  \t\n  Greeting Exchange  \n\t  ")),
                    std::nullopt)));
          });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->event->is_conversation_title_event());
  EXPECT_EQ(result->event->get_conversation_title_event()->title,
            "Greeting Exchange");
}

TEST_F(EngineConsumerOAIUnitTest, GenerateConversationTitle_NullEvent) {
  auto* client = GetClient();
  PageContentsMap page_contents;

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "Hello",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt, nullptr));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Hi there!", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
      nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>&) {
            // Return result with null event
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    nullptr, std::nullopt)));
          });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateConversationTitle_NonCompletionEvent) {
  auto* client = GetClient();
  PageContentsMap page_contents;

  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "Hello",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt, nullptr));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Hi there!", std::nullopt, std::nullopt, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
      nullptr));

  base::test::TestFuture<EngineConsumer::GenerationResult> future;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions&, std::vector<OAIMessage> messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback,
             const std::optional<std::vector<std::string>>&) {
            // Return a non-completion event (e.g., tool use event)
            auto tool_use_event = mojom::ToolUseEvent::New();
            tool_use_event->id = "test_id";
            tool_use_event->tool_name = "test_tool";
            tool_use_event->arguments_json = "{}";
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewToolUseEvent(
                        std::move(tool_use_event)),
                    std::nullopt)));
          });

  engine_->GenerateConversationTitle(page_contents, history,
                                     future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateRewriteSuggestion_UnsupportedActionTypeReturnsInternalError) {
  auto* client = GetClient();

  // Expect PerformRequest is NOT called for unsupported action types
  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _)).Times(0);

  base::test::TestFuture<EngineConsumer::GenerationResult> future;
  engine_->GenerateRewriteSuggestion("Hello World",
                                     mojom::ActionType::CREATE_TAGLINE,
                                     base::DoNothing(), future.GetCallback());

  auto result = future.Take();
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), mojom::APIError::InternalError);

  testing::Mock::VerifyAndClearExpectations(client);
}

class EngineConsumerOAIUnitTest_GenerateRewrite
    : public EngineConsumerOAIUnitTest,
      public testing::WithParamInterface<GenerateRewriteTestParam> {};

TEST_P(EngineConsumerOAIUnitTest_GenerateRewrite, GenerateRewriteSuggestion) {
  GenerateRewriteTestParam params = GetParam();
  auto* client = GetClient();
  base::RunLoop run_loop;

  std::string test_text = "Hello World";
  std::string expected_response = "Improved text here.";
  std::string expected_seed =
      "Here is the requested rewritten version of "
      "the excerpt in <response> tags:\n<response>";

  // Expected messages in OAI format with l10n strings
  std::string expected_excerpt_text =
      l10n_util::GetStringFUTF8(IDS_AI_CHAT_LLAMA2_SELECTED_TEXT_PROMPT_SEGMENT,
                                base::UTF8ToUTF16(test_text));
  std::string expected_text =
      params.tone.has_value()
          ? l10n_util::GetStringFUTF8(params.message_id,
                                      base::UTF8ToUTF16(*params.tone))
          : l10n_util::GetStringUTF8(params.message_id);
  std::string expected_messages = absl::StrFormat(
      R"([
          {
            "role": "user",
            "content": [
              {"type": "text", "text": "%s"},
              {"type": "text", "text": "%s"}
            ]
          },
          {
            "role": "assistant",
            "content": [
              {"type": "text", "text": "%s"}
            ]
          }
        ])",
      expected_excerpt_text, expected_text, expected_seed);

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [&](const mojom::CustomModelOptions& options,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback data_callback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>& stop_sequences) {
            // Verify the messages structure
            ASSERT_EQ(messages.size(), 2u);

            // First message: user with page excerpt and rewrite action
            const auto& first_message = messages[0];
            EXPECT_EQ(first_message.role, "user");
            ASSERT_EQ(first_message.content.size(), 2u);
            ASSERT_EQ(first_message.content[0]->which(),
                      mojom::ContentBlock::Tag::kPageExcerptContentBlock);
            EXPECT_EQ(first_message.content[0]
                          ->get_page_excerpt_content_block()
                          ->text,
                      test_text);
            EXPECT_EQ(first_message.content[1]->which(),
                      params.expected_content_type);

            // Second message: assistant seed message
            const auto& second_message = messages[1];
            EXPECT_EQ(second_message.role, "assistant");
            ASSERT_EQ(second_message.content.size(), 1u);
            ASSERT_EQ(second_message.content[0]->which(),
                      mojom::ContentBlock::Tag::kTextContentBlock);
            EXPECT_EQ(second_message.content[0]->get_text_content_block()->text,
                      expected_seed);

            // Verify stop sequences include </response>
            ASSERT_TRUE(stop_sequences.has_value());
            ASSERT_EQ(stop_sequences->size(), 1u);
            EXPECT_EQ((*stop_sequences)[0], "</response>");

            // Verify serialized JSON format
            // This actually tests the serialization in API client, although
            // this unit test is mainly for engine_consumer, it is useful to
            // also test the client part here so we can make sure each logical
            // feature such as rewrite requests would produce the expected
            // serialized messages.
            EXPECT_EQ(client->GetMessagesJson(std::move(messages)),
                      FormatComparableMessagesJson(expected_messages));

            // Return completion
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    mojom::ConversationEntryEvent::NewCompletionEvent(
                        mojom::CompletionEvent::New(expected_response)),
                    std::nullopt /* model_key*/)));
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
    EngineConsumerOAIUnitTest_GenerateRewrite,
    testing::Values(
        GenerateRewriteTestParam{
            "Paraphrase", mojom::ActionType::PARAPHRASE,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock,
            IDS_AI_CHAT_QUESTION_PARAPHRASE, std::nullopt,
            mojom::SimpleRequestType::kParaphrase},
        GenerateRewriteTestParam{
            "Improve", mojom::ActionType::IMPROVE,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock,
            IDS_AI_CHAT_QUESTION_IMPROVE, std::nullopt,
            mojom::SimpleRequestType::kImprove},
        GenerateRewriteTestParam{
            "Shorten", mojom::ActionType::SHORTEN,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock,
            IDS_AI_CHAT_QUESTION_SHORTEN, std::nullopt,
            mojom::SimpleRequestType::kShorten},
        GenerateRewriteTestParam{
            "Expand", mojom::ActionType::EXPAND,
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock,
            IDS_AI_CHAT_QUESTION_EXPAND, std::nullopt,
            mojom::SimpleRequestType::kExpand},
        GenerateRewriteTestParam{
            "Academic", mojom::ActionType::ACADEMICIZE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock,
            IDS_AI_CHAT_QUESTION_CHANGE_TONE_TEMPLATE, "academic"},
        GenerateRewriteTestParam{
            "Professional", mojom::ActionType::PROFESSIONALIZE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock,
            IDS_AI_CHAT_QUESTION_CHANGE_TONE_TEMPLATE, "professional"},
        GenerateRewriteTestParam{
            "Casual", mojom::ActionType::CASUALIZE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock,
            IDS_AI_CHAT_QUESTION_CHANGE_TONE_TEMPLATE, "casual"},
        GenerateRewriteTestParam{
            "Funny", mojom::ActionType::FUNNY_TONE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock,
            IDS_AI_CHAT_QUESTION_CHANGE_TONE_TEMPLATE, "funny"},
        GenerateRewriteTestParam{
            "Persuasive", mojom::ActionType::PERSUASIVE_TONE,
            mojom::ContentBlock::Tag::kChangeToneContentBlock,
            IDS_AI_CHAT_QUESTION_CHANGE_TONE_TEMPLATE, "persuasive"}),
    [](const testing::TestParamInfo<GenerateRewriteTestParam>& info) {
      return info.param.name;
    });

TEST_F(EngineConsumerOAIUnitTest, GetSuggestedTopics) {
  auto [tabs, tabs_json_strings] =
      GetMockTabsAndExpectedTabsJsonString(2 * kTabListChunkSize, false);
  ASSERT_EQ(tabs.size(), 2 * kTabListChunkSize);
  ASSERT_EQ(tabs_json_strings.size(), 2u);

  auto* client = GetClient();

  // Compute expected localized texts for each chunk and
  // JSON-escape them for embedding in expected JSON templates.
  std::string chunk1_text =
      l10n_util::GetStringFUTF8(IDS_AI_CHAT_TAB_FOCUS_SUGGEST_TOPICS,
                                base::UTF8ToUTF16(tabs_json_strings[0]));
  std::string chunk1_escaped;
  base::EscapeJSONString(chunk1_text, false, &chunk1_escaped);

  std::string chunk2_text =
      l10n_util::GetStringFUTF8(IDS_AI_CHAT_TAB_FOCUS_SUGGEST_TOPICS,
                                base::UTF8ToUTF16(tabs_json_strings[1]));
  std::string chunk2_escaped;
  base::EscapeJSONString(chunk2_text, false, &chunk2_escaped);

  // Build expected dedupe topics JSON matching how
  // BuildOAIDedupeTopicsMessages serializes merged topics.
  base::ListValue topic_list;
  for (const auto& t : {"topic1", "topic2", "topic3", "topic7", "topic3",
                        "topic4", "topic5", "topic6"}) {
    topic_list.Append(t);
  }
  std::string dedupe_text = l10n_util::GetStringFUTF8(
      IDS_AI_CHAT_TAB_FOCUS_REDUCE_TOPICS,
      base::UTF8ToUTF16(base::WriteJson(topic_list).value_or("")));
  std::string dedupe_escaped;
  base::EscapeJSONString(dedupe_text, false, &dedupe_escaped);

  std::string expected_messages1 = absl::StrFormat(
      R"([{"role":"user","content":[{"type":"text","text":"%s"}]}])",
      chunk1_escaped);
  std::string expected_messages2 = absl::StrFormat(
      R"([{"role":"user","content":[{"type":"text","text":"%s"}]}])",
      chunk2_escaped);
  std::string expected_messages3 = absl::StrFormat(
      R"([{"role":"user","content":[{"type":"text","text":"%s"}]}])",
      dedupe_escaped);

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .Times(3)
      .WillOnce(
          [&](const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            EXPECT_EQ(client->GetMessagesJson(std::move(messages)),
                      FormatComparableMessagesJson(expected_messages1));
            auto completion_event =
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New(
                        "{ \"topics\": [\"topic1\", \"topic2\", \"topic3\", "
                        "\"topic7\"] }"));
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    std::move(completion_event), std::nullopt)));
          })
      .WillOnce(
          [&](const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            EXPECT_EQ(client->GetMessagesJson(std::move(messages)),
                      FormatComparableMessagesJson(expected_messages2));
            auto completion_event =
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New(
                        "{ \"topics\": [\n  \"topic3\",\n  \"topic4\",\n  "
                        "\"topic5\",\n  \"topic6\"\n] }"));
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    std::move(completion_event), std::nullopt)));
          })
      .WillOnce(
          [&](const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            EXPECT_EQ(client->GetMessagesJson(std::move(messages)),
                      FormatComparableMessagesJson(expected_messages3));
            auto completion_event =
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New(
                        "{ \"topics\": [\"topic1\", \"topic3\", \"topic4\", "
                        "\"topic5\", "
                        "\"topic7\"] }"));
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
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
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, GetSuggestedTopics_SingleTabChunk) {
  auto [tabs, tabs_json_strings] =
      GetMockTabsAndExpectedTabsJsonString(1, false);
  ASSERT_EQ(tabs.size(), 1u);
  ASSERT_EQ(tabs_json_strings.size(), 1u);

  auto* client = GetClient();

  std::string expected_text =
      l10n_util::GetStringFUTF8(IDS_AI_CHAT_TAB_FOCUS_SUGGEST_TOPICS_WITH_EMOJI,
                                base::UTF8ToUTF16(tabs_json_strings[0]));
  std::string expected_escaped;
  base::EscapeJSONString(expected_text, false, &expected_escaped);
  std::string expected_messages = absl::StrFormat(
      R"([{"role":"user","content":[{"type":"text","text":"%s"}]}])",
      expected_escaped);

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .WillOnce(
          [&](const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            EXPECT_EQ(client->GetMessagesJson(std::move(messages)),
                      FormatComparableMessagesJson(expected_messages));
            auto completion_event =
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New(
                        "{ \"topics\": [\n  \"topic1\",\n  "
                        "\"topic2\"\n] }"));
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    std::move(completion_event), std::nullopt)));
          });

  engine_->GetSuggestedTopics(
      tabs,
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, std::vector<std::string>({"topic1", "topic2"}));
      }));
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, GetFocusTabs) {
  auto [tabs, tabs_json_strings] =
      GetMockTabsAndExpectedTabsJsonString(2 * kTabListChunkSize, false);
  ASSERT_EQ(tabs.size(), 2 * kTabListChunkSize);
  ASSERT_EQ(tabs_json_strings.size(), 2u);

  auto* client = GetClient();

  std::string chunk1_text = l10n_util::GetStringFUTF8(
      IDS_AI_CHAT_TAB_FOCUS_FILTER_TABS,
      base::UTF8ToUTF16(tabs_json_strings[0]), u"test_topic");
  std::string chunk1_escaped;
  base::EscapeJSONString(chunk1_text, false, &chunk1_escaped);

  std::string chunk2_text = l10n_util::GetStringFUTF8(
      IDS_AI_CHAT_TAB_FOCUS_FILTER_TABS,
      base::UTF8ToUTF16(tabs_json_strings[1]), u"test_topic");
  std::string chunk2_escaped;
  base::EscapeJSONString(chunk2_text, false, &chunk2_escaped);

  std::string expected_messages1 = absl::StrFormat(
      R"([{"role":"user","content":[{"type":"text","text":"%s"}]}])",
      chunk1_escaped);
  std::string expected_messages2 = absl::StrFormat(
      R"([{"role":"user","content":[{"type":"text","text":"%s"}]}])",
      chunk2_escaped);

  EXPECT_CALL(*client, PerformRequest(_, _, _, _, _))
      .Times(2)
      .WillOnce(
          [&](const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            EXPECT_EQ(client->GetMessagesJson(std::move(messages)),
                      FormatComparableMessagesJson(expected_messages1));
            auto completion_event =
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New(
                        "{ \"tab_ids\": [\"id1\", \"id2\"] }"));
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
                    std::move(completion_event), std::nullopt)));
          })
      .WillOnce(
          [&](const mojom::CustomModelOptions&,
              std::vector<OAIMessage> messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback,
              const std::optional<std::vector<std::string>>&) {
            EXPECT_EQ(client->GetMessagesJson(std::move(messages)),
                      FormatComparableMessagesJson(expected_messages2));
            auto completion_event =
                mojom::ConversationEntryEvent::NewCompletionEvent(
                    mojom::CompletionEvent::New(
                        "{ \"tab_ids\": [\n  \"id75\",\n  "
                        "\"id76\"\n] }"));
            std::move(completed_callback)
                .Run(base::ok(EngineConsumer::GenerationResultData(
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
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, GetSuggestedTopics_EmptyTabs) {
  engine_->GetSuggestedTopics(
      {},
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        EXPECT_EQ(result, base::unexpected(mojom::APIError::InternalError));
      }));
}

TEST_F(EngineConsumerOAIUnitTest, GetFocusTabs_EmptyTabs) {
  engine_->GetFocusTabs(
      {}, "topic",
      base::BindLambdaForTesting([&](base::expected<std::vector<std::string>,
                                                    mojom::APIError> result) {
        EXPECT_EQ(result, base::unexpected(mojom::APIError::InternalError));
      }));
}

}  // namespace ai_chat

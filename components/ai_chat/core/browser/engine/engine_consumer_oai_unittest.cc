/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_oai.h"

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/containers/checked_iterators.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/i18n/time_formatting.h"
#include "base/memory/scoped_refptr.h"
#include "base/numerics/clamped_math.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/test_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "components/grit/brave_components_strings.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using ::testing::_;
using ::testing::Sequence;

namespace ai_chat {

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
               base::Value::List,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback),
              (override));
};

class EngineConsumerOAIUnitTest : public testing::Test {
 public:
  EngineConsumerOAIUnitTest() = default;
  ~EngineConsumerOAIUnitTest() override = default;

  void SetUp() override {
    auto options = mojom::CustomModelOptions::New();
    options->endpoint = GURL("https://test.com/");
    options->model_request_name = "request_name";
    options->context_size = 5000;
    options->max_associated_content_length = 17200;
    options->model_system_prompt = "This is a custom system prompt.";
    options->context_size = 5000;
    options->max_associated_content_length = 17200;
    options->api_key = "api_key";

    model_ = mojom::Model::New();
    model_->key = "test_model_key";
    model_->display_name = "Test Model Display Name";
    model_->options =
        mojom::ModelOptions::NewCustomModelOptions(std::move(options));

    engine_ = std::make_unique<EngineConsumerOAIRemote>(
        *model_->options->get_custom_model_options(), nullptr);

    engine_->SetAPIForTesting(std::make_unique<MockOAIAPIClient>());
  }

  MockOAIAPIClient* GetClient() {
    return static_cast<MockOAIAPIClient*>(engine_->GetAPIForTesting());
  }

  void TearDown() override {}

 protected:
  base::test::TaskEnvironment task_environment_;
  mojom::ModelPtr model_;
  std::unique_ptr<EngineConsumerOAIRemote> engine_;
};

TEST_F(EngineConsumerOAIUnitTest, UpdateModelOptions) {
  auto* client = GetClient();

  base::RunLoop run_loop;
  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce([&run_loop, this](
                    const mojom::CustomModelOptions& model_options,
                    base::Value::List, EngineConsumer::GenerationDataCallback,
                    EngineConsumer::GenerationCompletedCallback) {
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

  engine_->GenerateQuestionSuggestions(false, "Page content", "",
                                       base::NullCallback());
  run_loop.Run();

  base::RunLoop run_loop2;
  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce([&run_loop2](const mojom::CustomModelOptions& model_options,
                             base::Value::List,
                             EngineConsumer::GenerationDataCallback,
                             EngineConsumer::GenerationCompletedCallback) {
        EXPECT_EQ("https://updated-test.com/", model_options.endpoint.spec());
        run_loop2.Quit();
      });

  engine_->GenerateQuestionSuggestions(false, "Page content", "",
                                       base::NullCallback());
  run_loop2.Run();

  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, GenerateQuestionSuggestions) {
  std::string page_content = "This is a test page content";

  auto* client = GetClient();
  base::RunLoop run_loop;

  auto invoke_completion_callback = [](const std::string& result_string) {
    return [result_string](
               const mojom::CustomModelOptions&, base::Value::List,
               EngineConsumer::GenerationDataCallback,
               EngineConsumer::GenerationCompletedCallback completed_callback) {
      std::move(completed_callback)
          .Run(EngineConsumer::GenerationResult(result_string));
    };
  };

  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce(invoke_completion_callback("Returning non question format"))
      .WillOnce(invoke_completion_callback(
          "<question>Question 1</question><question>Question 2</question>"))
      .WillOnce(invoke_completion_callback(
          "<question>Question 1</question>\n\n<question>Question 2</question>"))
      .WillOnce(invoke_completion_callback(
          "< question>>Question 1<</question><question>Question 2</question "
          ">"));

  engine_->GenerateQuestionSuggestions(
      false, page_content, "",
      base::BindLambdaForTesting(
          [](EngineConsumer::SuggestedQuestionResult result) {
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ(result.value().size(), 1ull);
          }));

  engine_->GenerateQuestionSuggestions(
      false, page_content, "",
      base::BindLambdaForTesting(
          [](EngineConsumer::SuggestedQuestionResult result) {
            EXPECT_STREQ(result.value()[0].c_str(), "Question 1");
            EXPECT_STREQ(result.value()[1].c_str(), "Question 2");
          }));

  engine_->GenerateQuestionSuggestions(
      false, page_content, "",
      base::BindLambdaForTesting(
          [](EngineConsumer::SuggestedQuestionResult result) {
            EXPECT_STREQ(result.value()[0].c_str(), "Question 1");
            EXPECT_STREQ(result.value()[1].c_str(), "Question 2");
          }));

  engine_->GenerateQuestionSuggestions(
      false, page_content, "",
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::SuggestedQuestionResult result) {
            EXPECT_STREQ(result.value()[0].c_str(), "Question 1");
            EXPECT_STREQ(result.value()[1].c_str(), "Question 2");
            run_loop.Quit();
          }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest,
       GenerateAssistantResponseWithDefaultSystemPrompt) {
  // Create a set of options WITHOUT a custom system prompt.
  auto options = mojom::CustomModelOptions::New();
  options->endpoint = GURL("https://test.com/");
  options->model_request_name = "request_name";
  options->api_key = "api_key";

  // Build a new model with the prompt-less options.
  model_ = mojom::Model::New();
  model_->key = "test_model_key";
  model_->display_name = "Test Model Display Name";
  model_->options =
      mojom::ModelOptions::NewCustomModelOptions(std::move(options));

  // Create a new engine with the new model.
  engine_ = std::make_unique<EngineConsumerOAIRemote>(
      *model_->options->get_custom_model_options(), nullptr);
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
      mojom::CharacterType::HUMAN,                 // Author is the user
      mojom::ActionType::UNSPECIFIED,              // No specific action
      mojom::ConversationTurnVisibility::VISIBLE,  // Visible to the user
      human_input,                                 // User message
      std::nullopt,                                // No selected text
      std::nullopt,                                // No events
      base::Time::Now(),                           // Current time
      std::nullopt,                                // No message edits
      false                                        // Not from Brave SERP
      ));

  // Prepare to capture API client request
  auto* client = GetClient();
  auto run_loop = std::make_unique<base::RunLoop>();

  // Expect a single call to PerformRequest
  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce(
          [&expected_system_message, &human_input, &assistant_response](
              const mojom::CustomModelOptions, base::Value::List messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback) {
            // system role is added by the engine
            EXPECT_EQ(*messages[0].GetDict().Find("role"), "system");
            EXPECT_EQ(*messages[0].GetDict().Find("content"),
                      expected_system_message);

            EXPECT_EQ(*messages[1].GetDict().Find("role"), "user");
            EXPECT_EQ(*messages[1].GetDict().Find("content"), human_input);

            std::move(completed_callback)
                .Run(EngineConsumer::GenerationResult(assistant_response));
          });

  // Initiate the test
  engine_->GenerateAssistantResponse(
      /* is_video */ false, "", history, human_input, "", base::DoNothing(),
      base::BindLambdaForTesting([&run_loop, &assistant_response](
                                     EngineConsumer::GenerationResult result) {
        EXPECT_STREQ(result.value().c_str(), assistant_response.c_str());
        run_loop->Quit();
      }));

  // Run the test
  run_loop->Run();

  // Verify the expectations
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest,
       TestGenerateAssistantResponseWithCustomSystemPrompt) {
  EngineConsumer::ConversationHistory history;

  std::string human_input = "Which show is this catchphrase from?";
  std::string selected_text = "This is the way.";
  std::string assistant_input = "This is mandalorian.";
  std::string expected_system_message = "This is a custom system prompt.";

  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE, human_input, selected_text,
      std::nullopt, base::Time::Now(), std::nullopt, false));

  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, assistant_input, std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, false));

  auto* client = GetClient();
  auto run_loop = std::make_unique<base::RunLoop>();

  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce(
          [&expected_system_message, &assistant_input](
              const mojom::CustomModelOptions, base::Value::List messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback) {
            // system role is added by the engine
            EXPECT_EQ(*messages[0].GetDict().Find("role"), "system");
            EXPECT_EQ(*messages[0].GetDict().Find("content"),
                      expected_system_message);

            EXPECT_EQ(*messages[1].GetDict().Find("role"), "user");
            EXPECT_EQ(
                *messages[1].GetDict().Find("content"),
                "This is an excerpt of the page content:\n<excerpt>\nThis is "
                "the way.\n</excerpt>\n\nWhich show is this catchphrase from?");

            EXPECT_EQ(*messages[2].GetDict().Find("role"), "assistant");
            EXPECT_EQ(*messages[2].GetDict().Find("content"), assistant_input);

            EXPECT_EQ(*messages[3].GetDict().Find("role"), "user");
            EXPECT_EQ(*messages[3].GetDict().Find("content"),
                      "What's his name?");

            std::move(completed_callback)
                .Run(EngineConsumer::GenerationResult("I dont know"));
          });

  {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New();
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->text = "What's his name?";
    history.push_back(std::move(entry));
  }

  engine_->GenerateAssistantResponse(
      /* is_video */ false, "", history, "What's his name?", "",
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult result) {
            EXPECT_STREQ(result.value().c_str(), "I dont know");
            run_loop->Quit();
          }));

  run_loop->Run();
  testing::Mock::VerifyAndClearExpectations(client);

  // Test with a modified server reply.
  run_loop = std::make_unique<base::RunLoop>();
  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce([&expected_system_message](
                    const mojom::CustomModelOptions, base::Value::List messages,
                    EngineConsumer::GenerationDataCallback,
                    EngineConsumer::GenerationCompletedCallback
                        completed_callback) {
        // system role is added by the engine
        EXPECT_EQ(*messages[0].GetDict().Find("role"), "system");
        EXPECT_EQ(*messages[0].GetDict().Find("content"),
                  expected_system_message);

        EXPECT_EQ(*messages[1].GetDict().Find("role"), "user");
        EXPECT_EQ(*messages[1].GetDict().Find("content"),
                  "Which show is 'This is the way' from?");

        // Modified server reply should be used here.
        EXPECT_EQ(*messages[2].GetDict().Find("role"), "assistant");
        EXPECT_EQ(*messages[2].GetDict().Find("content"), "The Mandalorian.");

        EXPECT_EQ(*messages[3].GetDict().Find("role"), "user");
        EXPECT_EQ(*messages[3].GetDict().Find("content"),
                  "Is it related to a broader series?");

        std::move(completed_callback).Run(EngineConsumer::GenerationResult(""));
      });

  engine_->GenerateAssistantResponse(
      false, "", GetHistoryWithModifiedReply(), "test", "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult result) {
            run_loop->Quit();
          }));
  run_loop->Run();
  testing::Mock::VerifyAndClearExpectations(client);

  // Test with page content refine event.
  {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New(
        mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        mojom::ConversationTurnVisibility::VISIBLE, "", std::nullopt,
        std::vector<mojom::ConversationEntryEventPtr>{}, base::Time::Now(),
        std::nullopt, false);
    entry->events->push_back(
        mojom::ConversationEntryEvent::NewPageContentRefineEvent(
            mojom::PageContentRefineEvent::New()));
    history.push_back(std::move(entry));
  }
  run_loop = std::make_unique<base::RunLoop>();
  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions, base::Value::List messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback) {
            std::move(completed_callback)
                .Run(EngineConsumer::GenerationResult(""));
          });

  engine_->GenerateAssistantResponse(
      false, "This is my page.", GetHistoryWithModifiedReply(), "Who?", "",
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop->Quit(); }));
  run_loop->Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, GenerateAssistantResponseEarlyReturn) {
  EngineConsumer::ConversationHistory history;
  auto* client = GetClient();
  auto run_loop = std::make_unique<base::RunLoop>();
  EXPECT_CALL(*client, PerformRequest(_, _, _, _)).Times(0);
  engine_->GenerateAssistantResponse(
      false, "This is my page.", history, "Who?", "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult result) {
            run_loop->Quit();
          }));
  run_loop->Run();
  testing::Mock::VerifyAndClearExpectations(client);

  mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "", std::nullopt,
      std::vector<mojom::ConversationEntryEventPtr>{}, base::Time::Now(),
      std::nullopt, false);
  entry->events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("Me")));
  history.push_back(std::move(entry));

  EXPECT_CALL(*client, PerformRequest(_, _, _, _)).Times(0);
  run_loop = std::make_unique<base::RunLoop>();
  engine_->GenerateAssistantResponse(
      false, "This is my page.", history, "Who?", "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult result) {
            run_loop->Quit();
          }));
  run_loop->Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, SummarizePage) {
  auto* client = GetClient();
  base::RunLoop run_loop;

  EngineConsumer::ConversationHistory history;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce(
          [](const mojom::CustomModelOptions, base::Value::List messages,
             EngineConsumer::GenerationDataCallback,
             EngineConsumer::GenerationCompletedCallback completed_callback) {
            // Page content should always be attached to the first message
            EXPECT_EQ(*messages[1].GetDict().Find("role"), "user");
            EXPECT_EQ(*messages[1].GetDict().Find("content"),
                      "This is the text of a web page:\n<page>\nThis is a "
                      "page.\n</page>\n\n");
            EXPECT_EQ(*messages[2].GetDict().Find("role"), "user");
            EXPECT_EQ(*messages[2].GetDict().Find("content"),
                      "Tell me more about this page");
            std::move(completed_callback)
                .Run(EngineConsumer::GenerationResult(""));
          });

  {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New();
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->text = "Tell me more about this page";
    history.push_back(std::move(entry));
  }

  engine_->GenerateAssistantResponse(
      /* is_video */ false,
      /* page_content */ "This is a page.", history, "", "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

}  // namespace ai_chat

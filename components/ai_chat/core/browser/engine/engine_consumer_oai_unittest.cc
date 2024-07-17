/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_oai.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/i18n/time_formatting.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

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
      .WillOnce([&](const mojom::CustomModelOptions& model_options,
                    base::Value::List, EngineConsumer::GenerationDataCallback,
                    EngineConsumer::GenerationCompletedCallback) {
        EXPECT_EQ("https://test.com/", model_options.endpoint.spec());

        // Update the model options
        auto options = mojom::CustomModelOptions::New();
        options->endpoint = GURL("https://updated-test.com");
        options->model_request_name = "request_name";
        options->api_key = "api_key";

        model_ = mojom::Model::New();
        model_->key = "test_model_key";
        model_->display_name = "Test Model Display Name";
        model_->options =
            mojom::ModelOptions::NewCustomModelOptions(std::move(options));
        engine_->UpdateModelOptions(*model_->options);

        run_loop.Quit();
      });

  engine_->GenerateQuestionSuggestions(false, "Page content",
                                       base::NullCallback());
  run_loop.Run();

  base::RunLoop run_loop2;
  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce([&](const mojom::CustomModelOptions& model_options,
                    base::Value::List, EngineConsumer::GenerationDataCallback,
                    EngineConsumer::GenerationCompletedCallback) {
        EXPECT_EQ("https://updated-test.com/", model_options.endpoint.spec());
        run_loop2.Quit();
      });

  engine_->GenerateQuestionSuggestions(false, "Page content",
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
      false, page_content,
      base::BindLambdaForTesting(
          [&](EngineConsumer::SuggestedQuestionResult result) {
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ(result.value().size(), 1ull);
          }));

  engine_->GenerateQuestionSuggestions(
      false, page_content,
      base::BindLambdaForTesting(
          [&](EngineConsumer::SuggestedQuestionResult result) {
            EXPECT_STREQ(result.value()[0].c_str(), "Question 1");
            EXPECT_STREQ(result.value()[1].c_str(), "Question 2");
          }));

  engine_->GenerateQuestionSuggestions(
      false, page_content,
      base::BindLambdaForTesting(
          [&](EngineConsumer::SuggestedQuestionResult result) {
            EXPECT_STREQ(result.value()[0].c_str(), "Question 1");
            EXPECT_STREQ(result.value()[1].c_str(), "Question 2");
          }));

  engine_->GenerateQuestionSuggestions(
      false, page_content,
      base::BindLambdaForTesting(
          [&](EngineConsumer::SuggestedQuestionResult result) {
            EXPECT_STREQ(result.value()[0].c_str(), "Question 1");
            EXPECT_STREQ(result.value()[1].c_str(), "Question 2");
            run_loop.Quit();
          }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, TestGenerateAssistantResponse) {
  EngineConsumer::ConversationHistory history;

  std::string human_input = "Which show is this catchphrase from?";
  std::string selected_text = "This is the way.";
  std::string assistant_input = "This is mandalorian.";

  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE, human_input, selected_text,
      std::nullopt, base::Time::Now(), std::nullopt));

  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, assistant_input, std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt));

  std::string date_and_time_string =
      base::UTF16ToUTF8(TimeFormatFriendlyDateAndTime(base::Time::Now()));
  std::string expected_system_message =
      base::StrCat({base::ReplaceStringPlaceholders(
          l10n_util::GetStringUTF8(IDS_AI_CHAT_LLAMA2_SYSTEM_MESSAGE_GENERIC),
          {date_and_time_string}, nullptr)});

  auto* client = GetClient();
  base::RunLoop run_loop;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce(
          [&](const mojom::CustomModelOptions, base::Value::List messages,
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
      /* is_video */ false, "", history, "What's his name?", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult result) {
            EXPECT_STREQ(result.value().c_str(), "I dont know");
            run_loop.Quit();
          }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

TEST_F(EngineConsumerOAIUnitTest, SummarizePage) {
  auto* client = GetClient();
  base::RunLoop run_loop;

  EngineConsumer::ConversationHistory history;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce(
          [&](const mojom::CustomModelOptions, base::Value::List messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback) {
            // Page content should always be attached to the first message
            EXPECT_EQ(*messages[1].GetDict().Find("role"), "user");
            EXPECT_EQ(*messages[1].GetDict().Find("content"),
                      "This is the text of a web page:\n<page>\nThis is a "
                      "page.\n</page>\n\nTell me more about this page");
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
      /* page_content */ "This is a page.", history, "", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

}  // namespace ai_chat

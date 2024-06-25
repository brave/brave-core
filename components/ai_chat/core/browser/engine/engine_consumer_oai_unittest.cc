/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_oai.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

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
    options->endpoint = GURL("https://test.com");
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
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE,
      "Which show is this catchphrase from?", "This is the way.",
      std::nullopt));
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "The Mandalorian.",
      std::nullopt, std::nullopt));

  auto* client = GetClient();
  base::RunLoop run_loop;

  EXPECT_CALL(*client, PerformRequest(_, _, _, _))
      .WillOnce(
          [&](const mojom::CustomModelOptions& model_options,
              base::Value::List messages,
              EngineConsumer::GenerationDataCallback,
              EngineConsumer::GenerationCompletedCallback completed_callback) {
            EXPECT_EQ(*messages.front().GetDict().Find("role"), "system");
            std::move(completed_callback)
                .Run(EngineConsumer::GenerationResult(""));
          });

  {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New();
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->text = "What's his name?";
    entry->selected_text = "I'm groot.";
    history.push_back(std::move(entry));
  }

  engine_->GenerateAssistantResponse(
      false, "This is a page.", history, "What's his name?", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));

  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(client);
}

}  // namespace ai_chat

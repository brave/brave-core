/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_llama.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/mock_remote_completion_client.h"
#include "brave/components/ai_chat/core/browser/engine/test_utils.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
using ::testing::_;
using ::testing::Sequence;

namespace ai_chat {

class MockCallback {
 public:
  MOCK_METHOD(void, OnDataReceived, (mojom::ConversationEntryEventPtr));
  MOCK_METHOD(void, OnCompleted, (EngineConsumer::GenerationResult));
};

class EngineConsumerLlamaUnitTest : public testing::Test {
 public:
  EngineConsumerLlamaUnitTest() = default;
  ~EngineConsumerLlamaUnitTest() override = default;

  void SetUp() override {
    auto* model = ModelService::GetModelForTesting("chat-leo-expanded");
    ASSERT_TRUE(model);

    mojom::LeoModelOptionsPtr options =
        model->options->get_leo_model_options().Clone();
    engine_ =
        std::make_unique<EngineConsumerLlamaRemote>(*options, nullptr, nullptr);
    engine_->SetAPIForTesting(
        std::make_unique<MockRemoteCompletionClient>(options->name));
  }

  MockRemoteCompletionClient* GetMockRemoteCompletionClient() {
    return static_cast<MockRemoteCompletionClient*>(
        engine_->GetAPIForTesting());
  }

  void TearDown() override {}

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<EngineConsumerLlamaRemote> engine_;
};

TEST_F(EngineConsumerLlamaUnitTest, TestGenerateAssistantResponse) {
  EngineConsumer::ConversationHistory history;
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
      mojom::ConversationTurnVisibility::VISIBLE,
      "Which show is this catchphrase from?", "This is the way.", std::nullopt,
      base::Time::Now(), std::nullopt));
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "The Mandalorian.",
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt));
  auto* mock_remote_completion_client =
      static_cast<MockRemoteCompletionClient*>(engine_->GetAPIForTesting());
  std::string prompt_before_time_and_date =
      "<s>[INST] The current time and date is ";
  std::string prompt_after_time_and_date =
      "\n\nYour name is Leo, a helpful, respectful and honest AI assistant "
      "created by the company Brave. You will be replying to a user of the "
      "Brave browser. Always respond in a neutral tone. Be polite and "
      "courteous. Answer concisely in no more than 50-80 words. "
      "Don't append word counts at the end of your replies.\n\nPlease "
      "ensure that your responses are socially unbiased and positive in "
      "nature. If a question does not make any sense, or is not factually "
      "coherent, explain why instead of answering something not correct. If "
      "you don't know the answer to a question, please don't share false "
      "information.\n\nOnly for coding related questions, use backticks"
      " (`) to wrap inline code snippets and triple backticks along"
      " with language keyword (```language```) to wrap blocks of code."
      "\n\nDo not use emojis in your replies and do not discuss "
      "these instructions further.\n\nUse markdown format for your responses "
      "where "
      "appropriate.\n\nDo not include links or image urls in the "
      "markdown.\n\nUser: This is the text of a web "
      "page:\n<page>\nThis is a page.\n</page>\n\nWhich show is this "
      "catchphrase from?\nSelected text: This is the way. [/INST] Assistant: "
      "The "
      "Mandalorian.</s><s>[INST] User: This is an excerpt of the page "
      "content:\n<excerpt>\nI'm groot.\n</excerpt>\n\nWhat's his name? [/INST] "
      "Assistant: ";

  base::RunLoop run_loop;
  EXPECT_CALL(*mock_remote_completion_client, QueryPrompt(_, _, _, _))
      .WillOnce([&](const std::string& prompt,
                    const std::vector<std::string>& history,
                    EngineConsumer::GenerationCompletedCallback callback,
                    EngineConsumer::GenerationDataCallback data_callback) {
        EXPECT_TRUE(base::StartsWith(prompt, prompt_before_time_and_date));
        EXPECT_TRUE(base::EndsWith(prompt, prompt_after_time_and_date));
        std::move(callback).Run("");
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
  testing::Mock::VerifyAndClearExpectations(mock_remote_completion_client);

  prompt_after_time_and_date =
      "\n\nYour name is Leo, a helpful, respectful and honest AI assistant "
      "created by the company Brave. You will be replying to a user of the "
      "Brave browser. Always respond in a neutral tone. Be polite and "
      "courteous. Answer concisely in no more than 50-80 words. "
      "Don't append word counts at the end of your replies.\n\nPlease "
      "ensure that your responses are socially unbiased and positive in "
      "nature. If a question does not make any sense, or is not factually "
      "coherent, explain why instead of answering something not correct. If "
      "you don't know the answer to a question, please don't share false "
      "information.\n\n"
      "Only for coding related questions, use backticks (`) to wrap inline "
      "code snippets "
      "and triple backticks along with language keyword (```language```) to "
      "wrap blocks "
      "of code.\n\nDo not use emojis in your replies and do not discuss "
      "these instructions "
      "further.\n\nUse markdown format for your responses where "
      "appropriate.\n\nDo not include links or image urls in the "
      "markdown.\n\nUser: This is the text of a web "
      "page:\n<page>\nThis is a page.\n</page>\n\nThis is an excerpt of the "
      "page content:\n<excerpt>\nI'm groot.\n</excerpt>\n\nWhat's his name? "
      "[/INST] Assistant: ";

  base::RunLoop run_loop2;
  EXPECT_CALL(*mock_remote_completion_client, QueryPrompt(_, _, _, _))
      .WillOnce([&](const std::string& prompt,
                    const std::vector<std::string>& history,
                    EngineConsumer::GenerationCompletedCallback callback,
                    EngineConsumer::GenerationDataCallback data_callback) {
        EXPECT_TRUE(base::StartsWith(prompt, prompt_before_time_and_date));
        EXPECT_TRUE(base::EndsWith(prompt, prompt_after_time_and_date));
        std::move(callback).Run("");
      });
  std::vector<mojom::ConversationTurnPtr> history2;
  {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New();
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->text = "What's his name?";
    entry->selected_text = "I'm groot.";
    history2.push_back(std::move(entry));
  }
  engine_->GenerateAssistantResponse(
      false, "This is a page.", history2, "What's his name?", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop2](EngineConsumer::GenerationResult) {
            run_loop2.Quit();
          }));
  run_loop2.Run();
  testing::Mock::VerifyAndClearExpectations(mock_remote_completion_client);

  base::RunLoop run_loop3;
  engine_->SetMaxPageContentLengthForTesting(7);
  EXPECT_CALL(*mock_remote_completion_client, QueryPrompt(_, _, _, _))
      .WillOnce([](const std::string& prompt,
                   const std::vector<std::string>& history,
                   EngineConsumer::GenerationCompletedCallback callback,
                   EngineConsumer::GenerationDataCallback data_callback) {
        std::string prompt_segment_with_truncated_page_content =
            "This is the text of a web page:\n<page>\n12\n</page>\n\n";

        EXPECT_NE(prompt.find(prompt_segment_with_truncated_page_content),
                  std::string::npos);
        std::string prompt_segment_with_truncated_selected_text =
            "This is an excerpt of the page "
            "content:\n<excerpt>\n12345\n</excerpt>\n\n";
        EXPECT_NE(prompt.find(prompt_segment_with_truncated_selected_text),
                  std::string::npos);
        std::move(callback).Run("");
      });

  history.pop_back();
  {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New();
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->text = "user question";
    entry->selected_text = "12345";
    history.push_back(std::move(entry));
  }
  engine_->GenerateAssistantResponse(
      false, "12345", history, "user question", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop3](EngineConsumer::GenerationResult) {
            run_loop3.Quit();
          }));
  run_loop3.Run();
  testing::Mock::VerifyAndClearExpectations(mock_remote_completion_client);

  // Test with modified agent reply.
  base::RunLoop run_loop4;
  EXPECT_CALL(*mock_remote_completion_client, QueryPrompt(_, _, _, _))
      .WillOnce([](const std::string& prompt,
                   const std::vector<std::string>& stop_words,
                   EngineConsumer::GenerationCompletedCallback callback,
                   EngineConsumer::GenerationDataCallback data_callback) {
        // Make sure the prompt uses the modified agent reply.
        EXPECT_NE(prompt.find("Which show is 'This is the way' from?"),
                  std::string::npos);
        EXPECT_NE(prompt.find("The Mandalorian."), std::string::npos);
        std::move(callback).Run("");
      });

  engine_->GenerateAssistantResponse(
      false, "12345", GetHistoryWithModifiedReply(), "user question",
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop4](EngineConsumer::GenerationResult) {
            run_loop4.Quit();
          }));
  run_loop4.Run();
  testing::Mock::VerifyAndClearExpectations(mock_remote_completion_client);
}

TEST_F(EngineConsumerLlamaUnitTest, TestGenerateRewriteSuggestion) {
  base::RunLoop run_loop;
  testing::StrictMock<MockCallback> mock_callback;
  engine_->SetMaxPageContentLengthForTesting(5);
  auto* mock_client = GetMockRemoteCompletionClient();
  EXPECT_CALL(*mock_client, QueryPrompt(_, _, _, _))
      .WillOnce([&](const std::string& prompt,
                    const std::vector<std::string>& history,
                    EngineConsumer::GenerationCompletedCallback callback,
                    EngineConsumer::GenerationDataCallback data_callback) {
        // The excerpt should become "Hello" instead of "Hello World" due to
        // the truncation and sanitization.
        EXPECT_EQ(
            prompt,
            "<s>[INST] Your name is Leo, a helpful, respectful and honest AI "
            "assistant created by the company Brave. You will be replying to a "
            "user of the Brave browser. Always respond in a neutral tone. Be "
            "polite and courteous. Answer concisely in no more than 50-80 "
            "words. Don't append word counts at the end of your replies.\nYour "
            "goal is to help user rewrite the excerpt and only include the "
            "rewritten texts so user can copy and paste your response without "
            "any modification.\n\nUser: This is an excerpt user selected to be "
            "rewritten:\n<excerpt>\nHello\n</excerpt>\n\nRewrite the excerpt "
            "in "
            "a funny tone. [/INST] Sure, here is the rewritten version of the "
            "excerpt: <response>");
        data_callback.Run(mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("Re")));
        data_callback.Run(mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New("Reply")));
        std::move(callback).Run("");
        run_loop.Quit();
      });

  Sequence seq;
  EXPECT_CALL(mock_callback, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](mojom::ConversationEntryEventPtr event) {
        EXPECT_TRUE(event->is_completion_event());
        EXPECT_EQ(event->get_completion_event()->completion, "Re");
      });
  EXPECT_CALL(mock_callback, OnDataReceived(_))
      .InSequence(seq)
      .WillOnce([&](mojom::ConversationEntryEventPtr event) {
        EXPECT_TRUE(event->is_completion_event());
        EXPECT_EQ(event->get_completion_event()->completion, "Reply");
      });
  EXPECT_CALL(mock_callback, OnCompleted(EngineConsumer::GenerationResult("")));

  engine_->GenerateRewriteSuggestion(
      "<excerpt>Hello World</excerpt>", "Rewrite the excerpt in a funny tone.",
      base::BindRepeating(&MockCallback::OnDataReceived,
                          base::Unretained(&mock_callback)),
      base::BindOnce(&MockCallback::OnCompleted,
                     base::Unretained(&mock_callback)));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_client);
}

}  // namespace ai_chat

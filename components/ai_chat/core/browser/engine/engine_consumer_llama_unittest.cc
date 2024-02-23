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
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/mock_remote_completion_client.h"
#include "brave/components/ai_chat/core/browser/models.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ConversationHistory = std::vector<ai_chat::mojom::ConversationTurn>;
using ::testing::_;

namespace ai_chat {

class EngineConsumerLlamaUnitTest : public testing::Test {
 public:
  EngineConsumerLlamaUnitTest() = default;
  ~EngineConsumerLlamaUnitTest() override = default;

  void SetUp() override {
    auto* model = GetModel("chat-leo-expanded");
    ASSERT_TRUE(model);
    engine_ =
        std::make_unique<EngineConsumerLlamaRemote>(*model, nullptr, nullptr);
    engine_->SetAPIForTesting(
        std::make_unique<MockRemoteCompletionClient>(model->name));
  }

  void TearDown() override {}

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<EngineConsumerLlamaRemote> engine_;
};

TEST_F(EngineConsumerLlamaUnitTest, TestGenerateAssistantResponse) {
  ConversationHistory history = {
      {mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
       mojom::ConversationTurnVisibility::VISIBLE,
       "Which show is this catchphrase from?", "This is the way."},
      {mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
       mojom::ConversationTurnVisibility::VISIBLE, "The Mandalorian.",
       std::nullopt}};
  auto* mock_remote_completion_client =
      static_cast<MockRemoteCompletionClient*>(engine_->GetAPIForTesting());
  std::string prompt_before_time_and_date =
      "<s>[INST] <<SYS>>\nThe current time and date is ";
  std::string prompt_after_time_and_date =
      "\n\nYour name is Leo, a helpful, respectful and honest AI assistant "
      "created by the company Brave. You will be replying to a user of the "
      "Brave browser. Always respond in a neutral tone. Be polite and "
      "courteous. Answer concisely in no more than 50-80 words.\n\nPlease "
      "ensure that your responses are socially unbiased and positive in "
      "nature. If a question does not make any sense, or is not factually "
      "coherent, explain why instead of answering something not correct. If "
      "you don't know the answer to a question, please don't share false "
      "information.\n\nUse unicode symbols for formatting where appropriate. "
      "Use backticks (`) to wrap inline coding-related words and triple "
      "backticks along with language keyword (```language```) to wrap blocks "
      "of code or data.\n<</SYS>>\n\nThis is the text of a web "
      "page:\n<page>\nThis is a page.\n</page>\n\nWhich show is this "
      "catchphrase from?\nSelected text: This is the way. [/INST] The "
      "Mandalorian.</s><s>[INST] This is an excerpt of the page "
      "content:\n<excerpt>\nI'm groot.\n</excerpt>\n\nWhat's his name? [/INST]";

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
  engine_->GenerateAssistantResponse(
      false, "This is a page.", "I'm groot.", history, "What's his name?",
      base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop](EngineConsumer::GenerationResult) { run_loop.Quit(); }));
  run_loop.Run();
  testing::Mock::VerifyAndClearExpectations(mock_remote_completion_client);

  prompt_after_time_and_date =
      "\n\nYour name is Leo, a helpful, respectful and honest AI assistant "
      "created by the company Brave. You will be replying to a user of the "
      "Brave browser. Always respond in a neutral tone. Be polite and "
      "courteous. Answer concisely in no more than 50-80 words.\n\nPlease "
      "ensure that your responses are socially unbiased and positive in "
      "nature. If a question does not make any sense, or is not factually "
      "coherent, explain why instead of answering something not correct. If "
      "you don't know the answer to a question, please don't share false "
      "information.\n\nUse unicode symbols for formatting where appropriate. "
      "Use backticks (`) to wrap inline coding-related words and triple "
      "backticks along with language keyword (```language```) to wrap blocks "
      "of code or data.\n<</SYS>>\n\nThis is the text of a web "
      "page:\n<page>\nThis is a page.\n</page>\n\nThis is an excerpt of the "
      "page content:\n<excerpt>\nI'm groot.\n</excerpt>\n\nWhat's his name? "
      "[/INST] ";

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
  engine_->GenerateAssistantResponse(
      false, "This is a page.", "I'm groot.", {}, "What's his name?",
      base::DoNothing(),
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
  engine_->GenerateAssistantResponse(
      false, "12345", "12345", history, "user question", base::DoNothing(),
      base::BindLambdaForTesting(
          [&run_loop3](EngineConsumer::GenerationResult) {
            run_loop3.Quit();
          }));
  run_loop3.Run();
  testing::Mock::VerifyAndClearExpectations(mock_remote_completion_client);
}

}  // namespace ai_chat

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_MOCK_ENGINE_CONSUMER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_MOCK_ENGINE_CONSUMER_H_

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ai_chat {

class MockEngineConsumer : public EngineConsumer {
 public:
  MockEngineConsumer();
  ~MockEngineConsumer() override;

  MOCK_METHOD(void,
              GenerateQuestionSuggestions,
              (const bool& is_video,
               const std::string& page_content,
               const std::string& selected_language,
               SuggestedQuestionsCallback callback),
              (override));

  MOCK_METHOD(void,
              GenerateAssistantResponse,
              (const bool& is_video,
               const std::string& page_content,
               const ConversationHistory& conversation_history,
               const std::string& human_input,
               const std::string& selected_language,
               GenerationDataCallback data_received_callback,
               GenerationCompletedCallback completed_callback),
              (override));

  MOCK_METHOD(void,
              GenerateRewriteSuggestion,
              (std::string text,
               const std::string& question,
               const std::string& selected_language,
               GenerationDataCallback received_callback,
               GenerationCompletedCallback completed_callback),
              (override));

  MOCK_METHOD(void, SanitizeInput, (std::string & input), (override));

  MOCK_METHOD(void, ClearAllQueries, (), (override));

  bool SupportsDeltaTextResponses() const override {
    return supports_delta_text_responses_;
  }

  void SetSupportsDeltaTextResponses(bool supports_delta_text_responses) {
    supports_delta_text_responses_ = supports_delta_text_responses;
  }

  void UpdateModelOptions(const mojom::ModelOptions& options) override {}

 private:
  bool supports_delta_text_responses_ = false;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_MOCK_ENGINE_CONSUMER_H_

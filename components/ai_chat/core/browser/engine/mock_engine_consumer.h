// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_MOCK_ENGINE_CONSUMER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_MOCK_ENGINE_CONSUMER_H_

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ai_chat {

class MockEngineConsumer : public EngineConsumer {
 public:
  MockEngineConsumer();
  ~MockEngineConsumer() override;

  MOCK_METHOD(void,
              GenerateQuestionSuggestions,
              (PageContents page_contents,
               const std::string& selected_language,
               SuggestedQuestionsCallback callback),
              (override));

  MOCK_METHOD(void,
              GenerateAssistantResponse,
              (PageContentsMap && page_contents,
               const ConversationHistory& conversation_history,
               const std::string& selected_language,
               bool is_temporary_chat,
               const std::vector<base::WeakPtr<Tool>>& tools,
               std::optional<std::string_view> preferred_tool_name,
               mojom::ConversationCapability conversation_capability,
               bool enable_research,
               GenerationDataCallback data_received_callback,
               GenerationCompletedCallback completed_callback),
              (override));

  MOCK_METHOD(void,
              GenerateRewriteSuggestion,
              (const std::string& text,
               mojom::ActionType action_type,
               const std::string& selected_language,
               GenerationDataCallback received_callback,
               GenerationCompletedCallback completed_callback),
              (override));

  MOCK_METHOD(void,
              GenerateConversationTitle,
              (const PageContentsMap& page_contents,
               const ConversationHistory& conversation_history,
               GenerationCompletedCallback completed_callback),
              (override));

  MOCK_METHOD(void, SanitizeInput, (std::string & input), (override));

  MOCK_METHOD(void, ClearAllQueries, (), (override));

  MOCK_METHOD(void,
              GetSuggestedTopics,
              (const std::vector<Tab>&, GetSuggestedTopicsCallback),
              (override));
  MOCK_METHOD(void,
              GetFocusTabs,
              (const std::vector<Tab>&,
               const std::string&,
               GetFocusTabsCallback),
              (override));
  MOCK_METHOD(const std::string&, GetModelName, (), (const, override));

  MOCK_METHOD(bool, RequiresClientSideTitleGeneration, (), (const, override));

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

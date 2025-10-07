// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

class PrefService;

namespace ai_chat {
class Tool;

namespace mojom {
class ModelOptions;
}  // namespace mojom

class ModelService;

// Abstract class for using AI completion engines to generate various specific
// styles of completion. The engines could be local (invoked directly via a
// subclass) or remote (invoked via network requests).
class EngineConsumer {
 public:
  using SuggestedQuestionResult =
      base::expected<std::vector<std::string>, mojom::APIError>;
  using SuggestedQuestionsCallback =
      base::OnceCallback<void(SuggestedQuestionResult)>;

  struct GenerationResultData {
    GenerationResultData(mojom::ConversationEntryEventPtr event,
                         std::optional<std::string>&& model_key);
    ~GenerationResultData();

    GenerationResultData(GenerationResultData&&);
    GenerationResultData& operator=(GenerationResultData&&);
    GenerationResultData(const GenerationResultData&) = delete;
    GenerationResultData& operator=(const GenerationResultData&) = delete;

    bool operator==(const GenerationResultData&) const = default;

    mojom::ConversationEntryEventPtr event;
    std::optional<std::string> model_key;
  };

  using GenerationResult =
      base::expected<GenerationResultData, mojom::APIError>;

  using GenerationDataCallback =
      base::RepeatingCallback<void(GenerationResultData)>;

  using GenerationCompletedCallback =
      base::OnceCallback<void(GenerationResult)>;

  using ConversationHistory = std::vector<mojom::ConversationTurnPtr>;

  using GetSuggestedTopicsCallback = base::OnceCallback<void(
      base::expected<std::vector<std::string>, mojom::APIError>)>;
  using GetFocusTabsCallback = base::OnceCallback<void(
      base::expected<std::vector<std::string>, mojom::APIError>)>;

  static std::string GetPromptForEntry(const mojom::ConversationTurnPtr& entry);

  EngineConsumer(ModelService* model_service, PrefService* prefs);
  EngineConsumer(const EngineConsumer&) = delete;
  EngineConsumer& operator=(const EngineConsumer&) = delete;
  virtual ~EngineConsumer();

  virtual void GenerateQuestionSuggestions(
      PageContents page_contents,
      const std::string& selected_language,
      SuggestedQuestionsCallback callback) = 0;

  virtual void GenerateAssistantResponse(
      PageContentsMap&& page_contents,
      const ConversationHistory& conversation_history,
      const std::string& selected_language,
      bool is_temporary_chat,
      const std::vector<base::WeakPtr<Tool>>& tools,
      std::optional<std::string_view> preferred_tool_name,
      mojom::ConversationCapability conversation_capability,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback) = 0;

  virtual void GenerateRewriteSuggestion(
      std::string text,
      const std::string& question,
      const std::string& selected_language,
      GenerationDataCallback received_callback,
      GenerationCompletedCallback completed_callback) {}

  // Generate a conversation title based on the conversation history.
  // Only called for engines that return true for
  // RequiresClientSideTitleGeneration(). Conversation history should ONLY
  // contain the first turn and completed assistant response.
  virtual void GenerateConversationTitle(
      const PageContentsMap& page_contents,
      const ConversationHistory& conversation_history,
      GenerationCompletedCallback completed_callback) {}

  // Prevent indirect prompt injections being sent to the AI model.
  // Include break-out strings contained in prompts, as well as the base
  // model command separators.
  virtual void SanitizeInput(std::string& input) = 0;

  // Stop any in-progress operations
  virtual void ClearAllQueries() = 0;

  // For streaming responses, whether the engine provides the entire completion
  // each time the callback is run (use |false|) or whether it provides a delta
  // from the previous run (use |true|).
  virtual bool SupportsDeltaTextResponses() const;

  // Whether this engine requires client-side conversation title generation.
  // Returns true for OAI engines, false for conversation API (server-side).
  virtual bool RequiresClientSideTitleGeneration() const;

  virtual void UpdateModelOptions(const mojom::ModelOptions& options) = 0;

  // Given a list of tabs, return a list of suggested topics from the server.
  virtual void GetSuggestedTopics(const std::vector<Tab>& tabs,
                                  GetSuggestedTopicsCallback callback) = 0;
  // Given a list of tabs and a specific topic, return a list of tabs to be
  // focused on from the server.
  virtual void GetFocusTabs(const std::vector<Tab>& tabs,
                            const std::string& topic,
                            GetFocusTabsCallback callback) = 0;
  virtual const std::string& GetModelName() const;

  void SetMaxAssociatedContentLengthForTesting(
      uint32_t max_associated_content_length) {
    max_associated_content_length_ = max_associated_content_length;
  }

  static std::string GetImageDataURL(base::span<uint8_t> image_data);
  static std::string GetPdfDataURL(base::span<uint8_t> pdf_data);

 protected:
  // Check if we should call GenerationCompletedCallback early based on the
  // conversation history. Ex. empty history, or if the last entry is not a
  // human message.
  bool CanPerformCompletionRequest(
      const ConversationHistory& conversation_history) const;

  static std::string BuildSmartModeDefinitionMessage(
      const mojom::SmartModeEntryPtr& smart_mode);
  uint32_t max_associated_content_length_ = 0;
  std::string model_name_ = "";
  raw_ptr<ModelService> model_service_;
  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_H_

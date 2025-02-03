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
#include "brave/components/ai_chat/core/browser/engine/remote_completion_client.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {
namespace mojom {
class ModelOptions;
}  // namespace mojom

// Abstract class for using AI completion engines to generate various specific
// styles of completion. The engines could be local (invoked directly via a
// subclass) or remote (invoked via network requests).
class EngineConsumer {
 public:
  using SuggestedQuestionResult =
      base::expected<std::vector<std::string>, mojom::APIError>;
  using SuggestedQuestionsCallback =
      base::OnceCallback<void(SuggestedQuestionResult)>;

  using GenerationResult = base::expected<std::string, mojom::APIError>;

  using GenerationDataCallback =
      base::RepeatingCallback<void(mojom::ConversationEntryEventPtr)>;

  using GenerationCompletedCallback =
      base::OnceCallback<void(GenerationResult)>;

  using ConversationHistory = std::vector<mojom::ConversationTurnPtr>;

  static std::string GetPromptForEntry(const mojom::ConversationTurnPtr& entry);

  EngineConsumer();
  EngineConsumer(const EngineConsumer&) = delete;
  EngineConsumer& operator=(const EngineConsumer&) = delete;
  virtual ~EngineConsumer();

  virtual void GenerateQuestionSuggestions(
      const bool& is_video,
      const std::string& page_content,
      const std::string& selected_language,
      SuggestedQuestionsCallback callback) = 0;

  virtual void GenerateAssistantResponse(
      const bool& is_video,
      const std::string& page_content,
      const ConversationHistory& conversation_history,
      const std::string& selected_language,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback) = 0;

  virtual void GenerateRewriteSuggestion(
      std::string text,
      const std::string& question,
      const std::string& selected_language,
      GenerationDataCallback received_callback,
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

  virtual void UpdateModelOptions(const mojom::ModelOptions& options) = 0;

  void SetMaxAssociatedContentLengthForTesting(
      uint32_t max_associated_content_length) {
    max_associated_content_length_ = max_associated_content_length;
  }

 protected:
  // Check if we should call GenerationCompletedCallback early based on the
  // conversation history. Ex. empty history, or if the last entry is not a
  // human message.
  bool CanPerformCompletionRequest(
      const ConversationHistory& conversation_history) const;
  uint32_t max_associated_content_length_ = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_H_

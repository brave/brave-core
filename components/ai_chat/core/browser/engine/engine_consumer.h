// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/engine/remote_completion_client.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

// Abstract class for using AI completion engines to generate various specific
// styles of completion. The engines could be local (invoked directly via a
// subclass) or remote (invoked via network requests).
class EngineConsumer {
 public:
  using SuggestedQuestionsCallback =
      base::OnceCallback<void(std::vector<std::string>)>;

  using GenerationResult = base::expected<std::string, mojom::APIError>;

  using GenerationDataCallback = base::RepeatingCallback<void(std::string)>;

  using GenerationCompletedCallback =
      base::OnceCallback<void(GenerationResult)>;

  using ConversationHistory = std::vector<mojom::ConversationTurn>;

  EngineConsumer();
  EngineConsumer(const EngineConsumer&) = delete;
  EngineConsumer& operator=(const EngineConsumer&) = delete;
  virtual ~EngineConsumer();

  virtual void GenerateQuestionSuggestions(
      const bool& is_video,
      const std::string& page_content,
      SuggestedQuestionsCallback callback) = 0;

  virtual void GenerateAssistantResponse(
      const bool& is_video,
      const std::string& page_content,
      std::optional<std::string> selected_text,
      const ConversationHistory& conversation_history,
      const std::string& human_input,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback) = 0;

  // Prevent indirect prompt injections being sent to the AI model.
  // Include break-out strings contained in prompts, as well as the base
  // model command separators.
  virtual void SanitizeInput(std::string& input) = 0;

  virtual void ClearAllQueries() = 0;

  void SetAPIForTesting(
      std::unique_ptr<RemoteCompletionClient> api_for_testing) {
    api_ = std::move(api_for_testing);
  }
  RemoteCompletionClient* GetAPIForTesting() { return api_.get(); }

  void SetMaxPageContentLengthForTesting(int max_page_content_length) {
    max_page_content_length_ = max_page_content_length;
  }

 protected:
  std::unique_ptr<RemoteCompletionClient> api_ = nullptr;
  int max_page_content_length_ = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_CONSUMER_H_

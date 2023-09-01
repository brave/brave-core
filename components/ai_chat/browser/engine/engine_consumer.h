// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_BROWSER_ENGINE_ENGINE_CONSUMER_H_
#define BRAVE_COMPONENTS_AI_CHAT_BROWSER_ENGINE_ENGINE_CONSUMER_H_

#include "base/functional/callback_forward.h"

#include "base/types/expected.h"
#include "string"
#include "vector"

#include "base/values.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

// Abstract class for using AI engines to generate various specifi styles of
// completion. The engines could be local (invoked directly via a subclass) or
// remote (invoked via network requests).
class EngineConsumer {
 public:
  using SuggestedQuestionsCallback = base::OnceCallback<void(std::vector<std::string>)>;
  using AssistantResponseResult = base::expected<std::string, mojom::APIError>;
  using SubmitHumanInputDataReceivedCallback = base::RepeatingCallback<void(std::string)>;
  using SubmitHumanInputCompletedCallback = base::OnceCallback<void(AssistantResponseResult)>;
  using ConversationHistory = std::vector<mojom::ConversationTurn>;

  EngineConsumer() = default;
  EngineConsumer(const EngineConsumer&) = delete;
  EngineConsumer& operator=(const EngineConsumer&) = delete;
  virtual ~EngineConsumer() = default;

  virtual void GenerateQuestionSuggestions(
      const bool& is_video,
      const std::string& page_content,
      SuggestedQuestionsCallback callback) = 0;
  virtual void SubmitHumanInput(const bool& is_video,
                                const std::string& page_content,
                                const ConversationHistory& conversation_history,
                                const std::string& human_input,
                                SubmitHumanInputDataReceivedCallback data_received_callback,
                                SubmitHumanInputCompletedCallback completed_callback) = 0;
  virtual void SanitizeInput(std::string& input) = 0;
  virtual void ClearAllQueries() = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_BROWSER_ENGINE_ENGINE_CONSUMER_H_

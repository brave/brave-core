/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_BROWSER_ENGINE_REMOTE_COMPLETION_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_BROWSER_ENGINE_REMOTE_COMPLETION_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

using api_request_helper::APIRequestResult;

// TODO(petemill): Is this meant to be shared by both Claude and Llama? It's not
// used to start the llama prompts but it is for Claude, but it's set for both
// as a stop sequence (and currently the only stop sequence used by the
// conversation prompts.
constexpr char kHumanPrompt[] = "Human:";

class RemoteCompletionClient {
 public:
  static std::string GetHumanPromptSegment();

  explicit RemoteCompletionClient(
      std::string model_name,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  RemoteCompletionClient(const RemoteCompletionClient&) = delete;
  RemoteCompletionClient& operator=(const RemoteCompletionClient&) = delete;
  ~RemoteCompletionClient();

  // This function queries both types of APIs: SSE and non-SSE.
  // In non-SSE cases, only the data_completed_callback will be triggered.
  void QueryPrompt(
      const std::string& prompt,
      const std::vector<std::string> stop_sequences,
      EngineConsumer::GenerationCompletedCallback data_completed_callback,
      EngineConsumer::GenerationDataCallback data_received_callback =
          base::NullCallback());
  // Clears all in-progress requests
  void ClearAllQueries();

 private:
  void OnQueryDataReceived(EngineConsumer::GenerationDataCallback callback,
                           base::expected<base::Value, std::string> result);
  void OnQueryCompleted(EngineConsumer::GenerationCompletedCallback callback,
                        APIRequestResult result);

  std::string model_name_;
  std::vector<std::string> default_stop_sequences_;
  api_request_helper::APIRequestHelper api_request_helper_;

  base::WeakPtrFactory<RemoteCompletionClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_BROWSER_ENGINE_REMOTE_COMPLETION_CLIENT_H_

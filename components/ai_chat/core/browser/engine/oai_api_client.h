// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_API_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_API_CLIENT_H_

#include <memory>
#include <string>
#include <utility>

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace api_request_helper {
class APIRequestResult;
}  // namespace api_request_helper

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

// Performs remote request to the OAI format APIs.
class OAIAPIClient {
 public:
  using GenerationResult = base::expected<std::string, mojom::APIError>;
  using GenerationDataCallback =
      base::RepeatingCallback<void(mojom::ConversationEntryEventPtr)>;
  using GenerationCompletedCallback =
      base::OnceCallback<void(GenerationResult)>;

  explicit OAIAPIClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  OAIAPIClient(const OAIAPIClient&) = delete;
  OAIAPIClient& operator=(const OAIAPIClient&) = delete;
  virtual ~OAIAPIClient();

  virtual void PerformRequest(const mojom::CustomModelOptions& model_options,
                              base::Value::List messages,
                              GenerationDataCallback data_received_callback,
                              GenerationCompletedCallback completed_callback);

  void ClearAllQueries();

 protected:
  void SetAPIRequestHelperForTesting(
      std::unique_ptr<api_request_helper::APIRequestHelper> api_helper) {
    api_request_helper_ = std::move(api_helper);
  }
  api_request_helper::APIRequestHelper* GetAPIRequestHelperForTesting() {
    return api_request_helper_.get();
  }

 private:
  void OnQueryCompleted(GenerationCompletedCallback callback,
                        APIRequestResult result);
  void OnQueryDataReceived(GenerationDataCallback callback,
                           base::expected<base::Value, std::string> result);

  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::WeakPtrFactory<OAIAPIClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_API_CLIENT_H_

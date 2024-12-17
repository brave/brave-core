// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_BYOM_API_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_BYOM_API_CLIENT_H_

#include <memory>
#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/remote_completion_client.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/api_request_helper/api_request_helper.h"

namespace api_request_helper {
class APIRequestResult;
}  // namespace api_request_helper

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {
namespace mojom {
class CustomModelOptions;
}  // namespace mojom

// Performs remote request to BYOM endpoints.
class BYOMAPIClient {
 public:
  using GenerationResult = base::expected<std::string, mojom::APIError>;
  using GenerationDataCallback =
      base::RepeatingCallback<void(mojom::ConversationEntryEventPtr)>;
  using GenerationCompletedCallback =
      base::OnceCallback<void(GenerationResult)>;

  explicit BYOMAPIClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  BYOMAPIClient(const BYOMAPIClient&) = delete;
  BYOMAPIClient& operator=(const BYOMAPIClient&) = delete;
  virtual ~BYOMAPIClient();

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

  base::WeakPtrFactory<BYOMAPIClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_BYOM_API_CLIENT_H_

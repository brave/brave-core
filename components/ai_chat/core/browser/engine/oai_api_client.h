// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_API_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_API_CLIENT_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/api_request_helper/api_request_helper.h"

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
  using GenerationResult = EngineConsumer::GenerationResult;
  using GenerationDataCallback = EngineConsumer::GenerationDataCallback;
  using GenerationCompletedCallback =
      EngineConsumer::GenerationCompletedCallback;

  explicit OAIAPIClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  OAIAPIClient(const OAIAPIClient&) = delete;
  OAIAPIClient& operator=(const OAIAPIClient&) = delete;
  virtual ~OAIAPIClient();

  // |model_options| must hold a CustomModelOptions variant.
  virtual void PerformRequest(
      const mojom::ModelOptions& model_options,
      std::vector<OAIMessage> messages,
      std::optional<base::ListValue> oai_tool_definitions,
      GenerationDataCallback data_received_callback,
      GenerationCompletedCallback completed_callback,
      const std::optional<std::vector<std::string>>& stop_sequences =
          std::nullopt);

  virtual void ClearAllQueries();

  static base::ListValue SerializeOAIMessages(std::vector<OAIMessage> messages);

 protected:
  // Builds a JSON request body for the OAI chat completions API.
  static std::string CreateJSONRequestBody(
      base::ListValue messages,
      bool is_sse_enabled,
      const std::string& model_request_name,
      std::optional<base::ListValue> oai_tool_definitions,
      const std::optional<std::vector<std::string>>& stop_sequences);

  // Maps an HTTP response code to an APIError using the same conventions as
  // OAI / Anthropic.
  static mojom::APIError MapResponseCodeToError(int response_code);

  // Parses and dispatches a single SSE chunk value. Handles both completion
  // text and tool calls. Safe to call with a null/non-dict value.
  static void OnQueryDataReceived(
      GenerationDataCallback callback,
      std::optional<std::string> model_key,
      std::optional<bool> is_near_verified,
      base::expected<base::Value, std::string> result);

  // Dispatches the final completion callback. If |success| is false,
  // |response_code| is mapped to an error. Otherwise |value| is the parsed
  // JSON body, or nullopt if parsing failed or the body was empty.
  static void HandleCompletion(GenerationCompletedCallback callback,
                               bool success,
                               int response_code,
                               std::optional<std::string> model_key,
                               std::optional<bool> is_near_verified,
                               std::optional<base::Value> value);

  void SetAPIRequestHelperForTesting(
      std::unique_ptr<api_request_helper::APIRequestHelper> api_helper) {
    api_request_helper_ = std::move(api_helper);
  }
  api_request_helper::APIRequestHelper* GetAPIRequestHelperForTesting() {
    return api_request_helper_.get();
  }

 private:
  void OnQueryCompleted(GenerationCompletedCallback callback,
                        api_request_helper::APIRequestResult result);

  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::WeakPtrFactory<OAIAPIClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_API_CLIENT_H_

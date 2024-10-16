// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ON_DEVICE_OAI_API_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ON_DEVICE_OAI_API_CLIENT_H_


#include <memory>
#include <string>
#include <utility>

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_oai.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace api_request_helper {
class APIRequestResult;
}  // namespace api_request_helper


namespace ai_chat {

// Performs remote request to the OAI format APIs.
class OnDeviceOAIAPIClient : public EngineConsumerOAIRemote::APIClient {
 public:
  explicit OnDeviceOAIAPIClient(
      mojo::Remote<mojom::OnDeviceModelWorker>& on_device_model_worker);

  OnDeviceOAIAPIClient(const OnDeviceOAIAPIClient&) = delete;
  OnDeviceOAIAPIClient& operator=(const OnDeviceOAIAPIClient&) = delete;
  ~OnDeviceOAIAPIClient() override;

  void PerformRequest(const mojom::ModelOptionsPtr& model_options,
                      base::Value::List messages,
                      EngineConsumer::GenerationDataCallback data_received_callback,
                      EngineConsumer::GenerationCompletedCallback completed_callback) override;
  void ClearAllQueries() override {}

 private:
  raw_ref<mojo::Remote<mojom::OnDeviceModelWorker>> on_device_model_worker_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ON_DEVICE_OAI_API_CLIENT_H_

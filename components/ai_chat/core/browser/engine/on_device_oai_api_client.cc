// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/on_device_oai_api_client.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/constants/brave_services_key.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

std::string CreateJSONRequestBody(
    base::Value::List messages,
    const bool is_sse_enabled) {
  base::Value::Dict dict;

  dict.Set("messages", std::move(messages));
  dict.Set("stream", is_sse_enabled);
  dict.Set("temperature", 0.7);

  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

class ResponseHandler : public mojom::OnDeviceModelResponseHandler {
 public:
  ResponseHandler(
      EngineConsumer::GenerationDataCallback data_received_callback,
      EngineConsumer::GenerationCompletedCallback completed_callback)
      : data_received_callback_(std::move(data_received_callback)),
        completed_callback_(std::move(completed_callback)) {}

  void OnPartialResponse(const std::string& delta_response) override {
    if (data_received_callback_.is_null() || delta_response.empty()) {
      return;
    }
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(delta_response));
    data_received_callback_.Run(std::move(event));
  }

  void OnComplete(const std::string& final_response) override {
    if (data_received_callback_.is_null()) {
      std::move(completed_callback_).Run(base::ok(final_response));
    } else {
      std::move(completed_callback_).Run(base::ok(std::string()));
    }
  }

  mojo::PendingRemote<mojom::OnDeviceModelResponseHandler> GetRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

 private:
  EngineConsumer::GenerationDataCallback data_received_callback_;
  EngineConsumer::GenerationCompletedCallback completed_callback_;

  mojo::Receiver<mojom::OnDeviceModelResponseHandler> receiver_{this};
};

}  // namespace

OnDeviceOAIAPIClient::OnDeviceOAIAPIClient(
   mojo::Remote<mojom::OnDeviceModelWorker>& on_device_model_worker): on_device_model_worker_(on_device_model_worker) { }

OnDeviceOAIAPIClient::~OnDeviceOAIAPIClient() = default;

void OnDeviceOAIAPIClient::PerformRequest(
    const mojom::ModelOptionsPtr& model_options,
    base::Value::List messages,
    EngineConsumer::GenerationDataCallback data_received_callback,
    EngineConsumer::GenerationCompletedCallback completed_callback) {


  if (!on_device_model_worker_->is_bound()) {
    std::move(completed_callback)
        .Run(base::unexpected(mojom::APIError::ConnectionIssue));
    return;
  }

  const bool is_sse_enabled =
      ai_chat::features::kAIChatSSE.Get() && !data_received_callback.is_null();
  const std::string request_body =
      CreateJSONRequestBody(std::move(messages), is_sse_enabled);

  auto response_handler = std::make_unique<ResponseHandler>(
      is_sse_enabled ? std::move(data_received_callback) : base::NullCallback(),
      std::move(completed_callback));

  auto remote = response_handler->GetRemote();

  on_device_model_worker_->get()->PerformRequest(
      std::move(request_body), std::move(remote),
      base::BindOnce(
          [](std::unique_ptr<ResponseHandler> response_handler,
             bool is_success) {
            DVLOG(2) << "Request completed: " << is_success;
          },
          std::move(response_handler)));
}

}  // namespace ai_chat

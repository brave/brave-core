// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oblivious_http_api_client.h"

#include <string_view>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace ai_chat {

namespace {

constexpr char kOHTTPRelayPathFormat[] = "v1/models/%s/relay";

constexpr std::string_view kSSEDataPrefix = "data: {";

constexpr base::TimeDelta kRequestTimeout = base::Minutes(5);

net::NetworkTrafficAnnotationTag GetOHTTPTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat_ohttp", R"cpp(
    semantics{
      sender : "AI Chat (OHTTP)" description :
          "Used to communicate with Brave AI Chat private inference models "
          "via Oblivious HTTP. The inner request body contains an "
          "OpenAI-compatible chat completion request and is encrypted to the "
          "OHTTP gateway. The relay (AI Chat server) only sees outer headers "
          "such as a Leo premium SKU credential and the model name." trigger :
              "Triggered by user interactions such as submitting an AI Chat "
              "conversation message when the selected model supports private "
              "inference."
      data :
          "Conversational messages input by the user as well as associated "
          "content. Encrypted via OHTTP between this client and the "
          "gateway. Outer headers include a Leo premium SKU credential "
          "and the Brave services key." destination : WEBSITE
    } policy{
      cookies_allowed : NO policy_exception_justification : "Not implemented."
    }
  )cpp");
}

bool IsStreamingEnabled(
    const EngineConsumer::GenerationDataCallback& data_received_callback) {
  return ai_chat::features::kAIChatSSE.Get() &&
         !data_received_callback.is_null();
}

void RunCompletedWithError(
    EngineConsumer::GenerationCompletedCallback completed_callback,
    mojom::APIError error) {
  std::move(completed_callback).Run(base::unexpected(error));
}

std::string GetModelKey(const std::string& model_name) {
  return "chat-" + model_name;
}

}  // namespace

ObliviousHttpAPIClient::InnerClient::InnerClient(
    CompletionCallback completion_callback,
    ChunkCallback chunk_callback)
    : completion_callback_(std::move(completion_callback)),
      chunk_callback_(std::move(chunk_callback)) {}

ObliviousHttpAPIClient::InnerClient::~InnerClient() = default;

mojo::PendingRemote<network::mojom::ObliviousHttpClient>
ObliviousHttpAPIClient::InnerClient::BindCompletionReceiver() {
  auto remote = completion_receiver_.BindNewPipeAndPassRemote();
  completion_receiver_.set_disconnect_handler(
      base::BindOnce(&InnerClient::OnPipeDisconnected, base::Unretained(this)));
  return remote;
}

mojo::PendingRemote<network::mojom::ObliviousHttpChunkClient>
ObliviousHttpAPIClient::InnerClient::BindChunkReceiver() {
  auto remote = chunk_receiver_.BindNewPipeAndPassRemote();
  chunk_receiver_.set_disconnect_handler(
      base::BindOnce(&InnerClient::OnPipeDisconnected, base::Unretained(this)));
  return remote;
}

void ObliviousHttpAPIClient::InnerClient::OnCompleted(
    network::mojom::ObliviousHttpCompletionResultPtr response) {
  if (completion_callback_.is_null()) {
    return;
  }
  int outer_response_code = net::HTTP_OK;
  int inner_response_code = 0;
  std::string body;
  switch (response->which()) {
    case network::mojom::ObliviousHttpCompletionResult::Tag::kNetError:
      outer_response_code = response->get_net_error();
      break;
    case network::mojom::ObliviousHttpCompletionResult::Tag::
        kOuterResponseErrorCode:
      outer_response_code = response->get_outer_response_error_code();
      break;
    case network::mojom::ObliviousHttpCompletionResult::Tag::kInnerResponse: {
      const auto& inner = response->get_inner_response();
      inner_response_code = inner->response_code;
      body = inner->response_body;
      break;
    }
  }
  std::move(completion_callback_)
      .Run(outer_response_code, inner_response_code, std::move(body));
}

void ObliviousHttpAPIClient::InnerClient::OnBodyChunk(
    const std::string& chunk) {
  if (!chunk_callback_.is_null()) {
    chunk_callback_.Run(chunk);
  }
}

void ObliviousHttpAPIClient::InnerClient::OnPipeDisconnected() {
  if (completion_callback_.is_null()) {
    return;
  }
  std::move(completion_callback_)
      .Run(net::ERR_FAILED, /*inner_response_code=*/0, std::string());
}

ObliviousHttpAPIClient::Request::Request(
    std::string model_name,
    std::string request_body,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback)
    : model_name(std::move(model_name)),
      request_body(std::move(request_body)),
      data_received_callback(std::move(data_received_callback)),
      completed_callback(std::move(completed_callback)) {}

ObliviousHttpAPIClient::Request::Request(Request&&) = default;

ObliviousHttpAPIClient::Request::~Request() = default;

ObliviousHttpAPIClient::ObliviousHttpAPIClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    network::NetworkContextGetter network_context_getter,
    AIChatCredentialManager* credential_manager,
    PrefService* profile_prefs)
    : OAIAPIClient(url_loader_factory),
      url_loader_factory_(url_loader_factory),
      network_context_getter_(std::move(network_context_getter)),
      credential_manager_(credential_manager),
      config_manager_(std::make_unique<ObliviousHttpConfigManager>(
          std::move(url_loader_factory),
          profile_prefs)),
      sequenced_task_runner_(base::ThreadPool::CreateSequencedTaskRunner({})) {}

ObliviousHttpAPIClient::~ObliviousHttpAPIClient() = default;

void ObliviousHttpAPIClient::PerformRequest(
    const mojom::ModelOptions& model_options,
    std::vector<OAIMessage> messages,
    std::optional<base::ListValue> oai_tool_definitions,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    const std::optional<std::vector<std::string>>& stop_sequences) {
  CHECK(model_options.is_leo_model_options());
  const auto& leo_opts = *model_options.get_leo_model_options();

  if (!credential_manager_) {
    return RunCompletedWithError(std::move(completed_callback),
                                 mojom::APIError::ConnectionIssue);
  }

  const bool is_streaming_enabled = IsStreamingEnabled(data_received_callback);

  std::string request_body = CreateJSONRequestBody(
      SerializeOAIMessages(std::move(messages)), is_streaming_enabled,
      leo_opts.name, std::move(oai_tool_definitions), stop_sequences);

  credential_manager_->FetchPremiumCredential(base::BindOnce(
      &ObliviousHttpAPIClient::OnCredentialFetched, weak_factory_.GetWeakPtr(),
      Request(leo_opts.name, std::move(request_body),
              std::move(data_received_callback),
              std::move(completed_callback))));
}

void ObliviousHttpAPIClient::ClearAllQueries() {
  weak_factory_.InvalidateWeakPtrs();
  config_manager_->CancelAll();
  inner_clients_.clear();
}

void ObliviousHttpAPIClient::OnCredentialFetched(
    Request request,
    std::optional<CredentialCacheEntry> credential) {
  const std::string model_name = request.model_name;
  config_manager_->RequestKeyConfig(
      model_name, base::BindOnce(&ObliviousHttpAPIClient::OnKeyConfigReady,
                                 weak_factory_.GetWeakPtr(), std::move(request),
                                 std::move(credential)));
}

void ObliviousHttpAPIClient::OnKeyConfigReady(
    Request request,
    std::optional<CredentialCacheEntry> credential,
    std::optional<ObliviousHttpConfigManager::KeyConfigResult>
        key_config_result) {
  if (!key_config_result) {
    if (credential.has_value()) {
      credential_manager_->PutCredentialInCache(std::move(*credential));
    }
    return RunCompletedWithError(std::move(request.completed_callback),
                                 mojom::APIError::ConnectionIssue);
  }
  DispatchOHTTPRequest(std::move(*key_config_result), std::move(request),
                       std::move(credential));
}

void ObliviousHttpAPIClient::DispatchOHTTPRequest(
    ObliviousHttpConfigManager::KeyConfigResult key_config_result,
    Request request,
    std::optional<CredentialCacheEntry> credential) {
  if (!network_context_getter_) {
    return RunCompletedWithError(std::move(request.completed_callback),
                                 mojom::APIError::ConnectionIssue);
  }
  network::mojom::NetworkContext* network_context =
      network_context_getter_.Run();
  if (!network_context) {
    return RunCompletedWithError(std::move(request.completed_callback),
                                 mojom::APIError::ConnectionIssue);
  }

  // Build the OHTTP request before transferring credential ownership into the
  // dispatch callback, since relay_url and relay_headers need to read it.
  auto ohttp_request = network::mojom::ObliviousHttpRequest::New();
  ohttp_request->relay_url = GetEndpointUrl(
      /*premium=*/credential.has_value(),
      absl::StrFormat(kOHTTPRelayPathFormat, request.model_name));
  ohttp_request->traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(GetOHTTPTrafficAnnotationTag());
  ohttp_request->timeout_duration = kRequestTimeout;
  ohttp_request->key_config = key_config_result.key_config;
  ohttp_request->resource_url = key_config_result.endpoint_url;
  ohttp_request->method = net::HttpRequestHeaders::kPostMethod;
  ohttp_request->request_body = network::mojom::ObliviousHttpRequestBody::New(
      std::move(request.request_body), "application/json");

  // Build outer (relay) request headers. These are sent to the relay in the
  // clear and are NOT encapsulated in the encrypted bhttp inner request.
  net::HttpRequestHeaders relay_headers;
  for (auto [name, value] : GetBraveHeaders(credential)) {
    relay_headers.SetHeader(name, std::move(value));
  }
  ohttp_request->relay_request_headers = std::move(relay_headers);
  ohttp_request->brave_services_key = BUILDFLAG(SERVICE_KEY_AICHAT);

  // Reserve a stable slot in the list so we can capture the iterator into
  // the dispatch callback before constructing the InnerClient.
  inner_clients_.emplace_back(nullptr);
  auto it = std::prev(inner_clients_.end());
  request.it = it;

  const bool chunking_enabled =
      IsStreamingEnabled(request.data_received_callback);
  GenerationDataCallback data_received_callback =
      request.data_received_callback;
  std::string model_name = request.model_name;
  *it = std::make_unique<InnerClient>(
      base::BindOnce(&ObliviousHttpAPIClient::OnInnerResponse,
                     weak_factory_.GetWeakPtr(), std::move(request),
                     std::move(credential)),
      base::BindRepeating(&ObliviousHttpAPIClient::OnInnerChunkReceived,
                          weak_factory_.GetWeakPtr(),
                          std::move(data_received_callback),
                          std::move(model_name)));

  InnerClient& inner_client = **it;

  if (chunking_enabled) {
    ohttp_request->enable_chunking = true;
    ohttp_request->chunk_client = inner_client.BindChunkReceiver();
  }

  network_context->GetViaObliviousHttp(std::move(ohttp_request),
                                       inner_client.BindCompletionReceiver());
}

void ObliviousHttpAPIClient::OnInnerResponse(
    Request request,
    std::optional<CredentialCacheEntry> credential,
    int outer_response_code,
    int inner_response_code,
    std::string response_body) {
  // Erase the InnerClient from the ownership list now that the request is done.
  inner_clients_.erase(request.it);

  // 401 outer response indicates an invalid credential; do not cache it.
  if (outer_response_code != net::HTTP_UNAUTHORIZED && credential.has_value()) {
    credential_manager_->PutCredentialInCache(std::move(*credential));
  }

  // A non-2xx outer response means the relay rejected the request; the cached
  // key config may be stale so clear it to force a fresh fetch next time.
  if (outer_response_code < 200 || outer_response_code >= 300) {
    config_manager_->ClearKeyConfig(request.model_name);
  }

  const bool success = inner_response_code >= 200 && inner_response_code < 300;
  sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::JSONReader::Read, std::move(response_body),
                     base::JSON_PARSE_RFC, static_cast<size_t>(200)),
      base::BindOnce(&OAIAPIClient::HandleCompletion,
                     std::move(request.completed_callback), success,
                     inner_response_code,
                     /*model_key=*/GetModelKey(request.model_name),
                     /*is_near_verified=*/true));
}

void ObliviousHttpAPIClient::OnInnerChunkReceived(
    GenerationDataCallback data_received_callback,
    std::string model_name,
    std::string chunk) {
  if (!base::StartsWith(chunk, kSSEDataPrefix)) {
    return;
  }
  // OHTTP chunks are guaranteed to contain complete JSON,
  // so there is no need to append chunks to a buffer and wait
  // for a full JSON object.
  sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::JSONReader::Read,
                     chunk.substr(kSSEDataPrefix.size() - 1),
                     base::JSON_PARSE_RFC, static_cast<size_t>(200)),
      base::BindOnce(&ObliviousHttpAPIClient::OnChunkParsed,
                     std::move(data_received_callback), std::move(model_name)));
}

// static
void ObliviousHttpAPIClient::OnChunkParsed(
    GenerationDataCallback data_received_callback,
    std::string model_name,
    std::optional<base::Value> value) {
  if (!value) {
    return;
  }
  OnQueryDataReceived(std::move(data_received_callback),
                      GetModelKey(model_name), /*is_near_verified=*/true,
                      base::ok(std::move(*value)));
}

}  // namespace ai_chat

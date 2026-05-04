// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/ohttp_api_client.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_v2_client.h"
#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"

namespace ai_chat {

namespace {

constexpr char kOhttpRelayPathPrefix[] = "v1/models/";
constexpr char kOhttpRelayPathSuffix[] = "/relay";

constexpr base::TimeDelta kRequestTimeout = base::Seconds(60);

net::NetworkTrafficAnnotationTag GetOhttpTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat_ohttp", R"cpp(
      semantics {
        sender: "AI Chat (OHTTP)"
        description:
          "Used to communicate with Brave AI Chat private inference models "
          "via Oblivious HTTP. The inner request body contains an "
          "OpenAI-compatible chat completion request and is encrypted to the "
          "OHTTP gateway. The relay (AI Chat server) only sees outer headers "
          "such as a Leo premium SKU credential and the model name."
        trigger:
          "Triggered by user interactions such as submitting an AI Chat "
          "conversation message when the selected model supports private "
          "inference."
        data:
          "Conversational messages input by the user as well as associated "
          "content. Encrypted via OHTTP between this client and the "
          "gateway. Outer headers include a Leo premium SKU credential "
          "and the Brave services key."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )cpp");
}

GURL GetOhttpRelayUrl(const std::string& model_name) {
  return GetEndpointUrl(/*premium=*/true,
                        kOhttpRelayPathPrefix + model_name + kOhttpRelayPathSuffix);
}

}  // namespace

OHTTPAPIClient::Request::Request(std::string model_name,
                                 std::string request_body,
                                 GenerationCompletedCallback completed_callback)
    : model_name(std::move(model_name)),
      request_body(std::move(request_body)),
      completed_callback(std::move(completed_callback)) {}

OHTTPAPIClient::Request::Request(Request&&) = default;

OHTTPAPIClient::Request::~Request() = default;

OHTTPAPIClient::InnerOhttpClient::InnerOhttpClient(DispatchCallback callback)
    : callback_(std::move(callback)) {}

OHTTPAPIClient::InnerOhttpClient::~InnerOhttpClient() = default;

void OHTTPAPIClient::InnerOhttpClient::OnCompleted(
    network::mojom::ObliviousHttpCompletionResultPtr response) {
  if (callback_.is_null()) {
    return;
  }
  int response_code = 0;
  std::string body;
  switch (response->which()) {
    case network::mojom::ObliviousHttpCompletionResult::Tag::kNetError:
      response_code = response->get_net_error();
      break;
    case network::mojom::ObliviousHttpCompletionResult::Tag::
        kOuterResponseErrorCode:
      response_code = response->get_outer_response_error_code();
      break;
    case network::mojom::ObliviousHttpCompletionResult::Tag::kInnerResponse: {
      const auto& inner = response->get_inner_response();
      response_code = inner->response_code;
      body = inner->response_body;
      break;
    }
  }
  std::move(callback_).Run(response_code, std::move(body));
}

OHTTPAPIClient::OHTTPAPIClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    network::NetworkContextGetter network_context_getter,
    AIChatCredentialManager* credential_manager,
    PrefService* profile_prefs)
    : OAIAPIClient(url_loader_factory),
      url_loader_factory_(url_loader_factory),
      network_context_getter_(std::move(network_context_getter)),
      credential_manager_(credential_manager),
      config_manager_(std::make_unique<OHTTPConfigManager>(
          std::move(url_loader_factory),
          profile_prefs)) {}

OHTTPAPIClient::~OHTTPAPIClient() = default;

void OHTTPAPIClient::PerformRequest(
    const mojom::ModelOptions& model_options,
    std::vector<OAIMessage> messages,
    std::optional<base::ListValue> oai_tool_definitions,
    GenerationDataCallback /*data_received_callback*/,
    GenerationCompletedCallback completed_callback,
    const std::optional<std::vector<std::string>>& stop_sequences) {
  CHECK(model_options.is_leo_model_options());
  const auto& leo_opts = *model_options.get_leo_model_options();

  std::string request_body =
      CreateJSONRequestBody(SerializeOAIMessages(std::move(messages)),
                            /*is_sse_enabled=*/false, leo_opts.name,
                            std::move(oai_tool_definitions), stop_sequences);

  if (!credential_manager_) {
    return RunCompletedWithError(std::move(completed_callback),
                                 mojom::APIError::ConnectionIssue);
  }

  credential_manager_->FetchPremiumCredential(
      base::BindOnce(&OHTTPAPIClient::OnCredentialFetched,
                     weak_factory_.GetWeakPtr(),
                     Request{leo_opts.name, std::move(request_body),
                             std::move(completed_callback)}));
}

void OHTTPAPIClient::ClearAllQueries() {
  weak_factory_.InvalidateWeakPtrs();
  config_manager_->CancelAll();
}

void OHTTPAPIClient::OnCredentialFetched(
    Request request,
    std::optional<CredentialCacheEntry> credential) {
  config_manager_->RequestKeyConfig(
      request.model_name,
      base::BindOnce(&OHTTPAPIClient::OnKeyConfigReady,
                     weak_factory_.GetWeakPtr(), std::move(request),
                     std::move(credential)));
}

void OHTTPAPIClient::OnKeyConfigReady(
    Request request,
    std::optional<CredentialCacheEntry> credential,
    std::optional<OHTTPConfigManager::KeyConfigResult> key_config_result) {
  if (!key_config_result) {
    if (credential.has_value()) {
      credential_manager_->PutCredentialInCache(std::move(*credential));
    }
    return RunCompletedWithError(std::move(request.completed_callback),
                                 mojom::APIError::ConnectionIssue);
  }
  DispatchOhttpRequest(std::move(*key_config_result), std::move(request),
                       std::move(credential));
}

void OHTTPAPIClient::DispatchOhttpRequest(
    OHTTPConfigManager::KeyConfigResult key_config_result,
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

  auto ohttp_request = network::mojom::ObliviousHttpRequest::New();
  ohttp_request->relay_url = GetOhttpRelayUrl(request.model_name);
  ohttp_request->traffic_annotation = net::MutableNetworkTrafficAnnotationTag(
      GetOhttpTrafficAnnotationTag());
  ohttp_request->timeout_duration = kRequestTimeout;
  ohttp_request->key_config = key_config_result.key_config;
  ohttp_request->resource_url = key_config_result.endpoint_url;
  ohttp_request->method = net::HttpRequestHeaders::kPostMethod;
  ohttp_request->request_body = network::mojom::ObliviousHttpRequestBody::New(
      std::move(request.request_body), "application/json");

  // Build outer (relay) request headers. These are sent to the relay in the
  // clear and are NOT encapsulated in the encrypted bhttp inner request.
  net::HttpRequestHeaders relay_headers;
  for (const auto& [name, value] :
       ConversationAPIV2Client::GetBraveHeaders(std::nullopt, std::nullopt,
                                               credential)) {
    relay_headers.SetHeader(name, value);
  }
  ohttp_request->relay_request_headers = std::move(relay_headers);
  ohttp_request->brave_services_key = BUILDFLAG(SERVICE_KEY_AICHAT);

  mojo::PendingRemote<network::mojom::ObliviousHttpClient> client_remote;
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<InnerOhttpClient>(base::BindOnce(
          &OHTTPAPIClient::OnInnerResponse, weak_factory_.GetWeakPtr(),
          std::move(request), std::move(credential))),
      client_remote.InitWithNewPipeAndPassReceiver());

  network_context->GetViaObliviousHttp(std::move(ohttp_request),
                                       std::move(client_remote));
}

void OHTTPAPIClient::OnInnerResponse(
    Request request,
    std::optional<CredentialCacheEntry> credential,
    int response_code,
    std::string response_body) {
  if (response_code >= 200 && response_code < 300) {
    // Recycle the credential for future requests.
    if (credential.has_value()) {
      credential_manager_->PutCredentialInCache(std::move(*credential));
    }

    auto value = base::JSONReader::Read(response_body, base::JSON_PARSE_RFC);
    if (value && value->is_dict()) {
      if (auto result = ParseOAICompletionResponse(value->GetDict(),
                                                   /*model_key=*/std::nullopt)) {
        std::move(request.completed_callback).Run(base::ok(std::move(*result)));
        return;
      }
    }
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(""));
    std::move(request.completed_callback)
        .Run(base::ok(EngineConsumer::GenerationResultData(
            std::move(event), /*model_key=*/std::nullopt)));
    return;
  }

  if (response_code != 401 && credential.has_value()) {
    // 401 indicates an invalid credential; do not return it to the cache.
    credential_manager_->PutCredentialInCache(std::move(*credential));
  }
  std::move(request.completed_callback)
      .Run(base::unexpected(MapResponseCodeToError(response_code)));
}

void OHTTPAPIClient::RunCompletedWithError(
    GenerationCompletedCallback completed_callback,
    mojom::APIError error) {
  std::move(completed_callback).Run(base::unexpected(error));
}

}  // namespace ai_chat

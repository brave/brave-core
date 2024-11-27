/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/remote_completion_client.h"

#include <base/containers/flat_map.h>

#include <ios>
#include <optional>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/field_trial_params.h"
#include "base/numerics/clamped_math.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {
namespace {

constexpr char kAIChatCompletionPath[] = "v2/complete";
constexpr char kHttpMethod[] = "POST";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to communicate with our partner API"
          "on behalf of the user interacting with the ChatUI."
        trigger:
          "Triggered by user sending a prompt."
        data:
          "Will generate a text that attempts to match the user gave it"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

base::Value::Dict CreateApiParametersDict(
    const std::string& prompt,
    const std::string& model_name,
    const base::flat_set<std::string_view>& stop_sequences,
    const std::vector<std::string>& additional_stop_sequences,
    const bool is_sse_enabled) {
  base::Value::Dict dict;

  base::Value::List all_stop_sequences;
  for (const auto& item : additional_stop_sequences) {
    all_stop_sequences.Append(item);
  }
  for (const auto& item : stop_sequences) {
    all_stop_sequences.Append(item);
  }

  const double temp = ai_chat::features::kAITemperature.Get();

  DCHECK(!model_name.empty());

  dict.Set("prompt", prompt);
  dict.Set("max_tokens_to_sample", 600);
  dict.Set("temperature", temp);
  dict.Set("top_k", -1);  // disabled
  dict.Set("top_p", 0.999);
  dict.Set("model", model_name);
  dict.Set("stop_sequences", std::move(all_stop_sequences));
  dict.Set("stream", is_sse_enabled);

  DVLOG(1) << __func__ << " Prompt: |" << prompt << "|\n";
  DVLOG(1) << __func__ << " Using model: " << model_name;

  return dict;
}

std::string CreateJSONRequestBody(base::ValueView node) {
  std::string json;
  base::JSONWriter::Write(node, &json);
  return json;
}

GURL GetEndpointUrl(bool premium, const std::string& path) {
  DCHECK(!path.starts_with("/"));

  auto* prefix = premium ? "ai-chat-premium.bsg" : "ai-chat.bsg";
  auto hostname = brave_domains::GetServicesDomain(
      prefix, brave_domains::ServicesEnvironment::DEV);

  GURL url{base::StrCat(
      {url::kHttpsScheme, url::kStandardSchemeSeparator, hostname, "/", path})};

  DCHECK(url.is_valid()) << "Invalid API Url: " << url.spec();

  return url;
}

}  // namespace

// static

RemoteCompletionClient::RemoteCompletionClient(
    const std::string& model_name,
    base::flat_set<std::string_view> stop_sequences,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager)
    : model_name_(model_name),
      stop_sequences_(std::move(stop_sequences)),
      api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          std::move(url_loader_factory)),
      credential_manager_(credential_manager) {}

RemoteCompletionClient::~RemoteCompletionClient() = default;

void RemoteCompletionClient::QueryPrompt(
    const std::string& prompt,
    std::vector<std::string> extra_stop_sequences,
    GenerationCompletedCallback data_completed_callback,
    GenerationDataCallback
        data_received_callback /* = base::NullCallback() */) {
  auto callback = base::BindOnce(
      &RemoteCompletionClient::OnFetchPremiumCredential,
      weak_ptr_factory_.GetWeakPtr(), prompt, std::move(extra_stop_sequences),
      std::move(data_completed_callback), std::move(data_received_callback));
  credential_manager_->FetchPremiumCredential(std::move(callback));
}

void RemoteCompletionClient::OnFetchPremiumCredential(
    const std::string& prompt,
    const std::vector<std::string>& extra_stop_sequences,
    GenerationCompletedCallback data_completed_callback,
    GenerationDataCallback data_received_callback,
    std::optional<CredentialCacheEntry> credential) {
  bool premium_enabled = credential.has_value();
  const GURL api_url = GetEndpointUrl(premium_enabled, kAIChatCompletionPath);
  const bool is_sse_enabled =
      ai_chat::features::kAIChatSSE.Get() && !data_received_callback.is_null();
  const base::Value::Dict& dict =
      CreateApiParametersDict(prompt, model_name_, stop_sequences_,
                              extra_stop_sequences, is_sse_enabled);
  const std::string request_body = CreateJSONRequestBody(dict);

  base::flat_map<std::string, std::string> headers;
  const auto digest_header = brave_service_keys::GetDigestHeader(request_body);
  headers.emplace(digest_header.first, digest_header.second);
  auto result = brave_service_keys::GetAuthorizationHeader(
      BUILDFLAG(SERVICE_KEY_AICHAT), headers, api_url, kHttpMethod, {"digest"});
  if (result) {
    std::pair<std::string, std::string> authorization_header = result.value();
    headers.emplace(authorization_header.first, authorization_header.second);
  }

  if (premium_enabled) {
    // Add Leo premium SKU credential as a Cookie header.
    std::string cookie_header_value =
        "__Secure-sku#brave-leo-premium=" + credential->credential;
    headers.emplace("Cookie", cookie_header_value);
  }
  headers.emplace("x-brave-key", BUILDFLAG(BRAVE_SERVICES_KEY));
  headers.emplace("Accept", "text/event-stream");

  if (is_sse_enabled) {
    VLOG(2) << "Making streaming AI Chat API Request";
    auto on_received = base::BindRepeating(
        &RemoteCompletionClient::OnQueryDataReceived,
        weak_ptr_factory_.GetWeakPtr(), std::move(data_received_callback));
    auto on_complete =
        base::BindOnce(&RemoteCompletionClient::OnQueryCompleted,
                       weak_ptr_factory_.GetWeakPtr(), credential,
                       std::move(data_completed_callback));

    api_request_helper_.RequestSSE(kHttpMethod, api_url, request_body,
                                   "application/json", std::move(on_received),
                                   std::move(on_complete), headers, {});
  } else {
    VLOG(2) << "Making non-streaming AI Chat API Request";
    auto on_complete =
        base::BindOnce(&RemoteCompletionClient::OnQueryCompleted,
                       weak_ptr_factory_.GetWeakPtr(), credential,
                       std::move(data_completed_callback));

    api_request_helper_.Request(kHttpMethod, api_url, request_body,
                                "application/json", std::move(on_complete),
                                headers, {});
  }
}

void RemoteCompletionClient::ClearAllQueries() {
  // TODO(nullhook): Keep track of in-progress requests and cancel them
  // individually. This would be useful to keep some in-progress requests alive.
  api_request_helper_.CancelAll();
}

void RemoteCompletionClient::OnQueryDataReceived(
    GenerationDataCallback callback,
    base::expected<base::Value, std::string> result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  // This client only supports completion events
  const std::string* completion = result->GetDict().FindString("completion");
  if (completion) {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(*completion));
    callback.Run(std::move(event));
  }
}

void RemoteCompletionClient::OnQueryCompleted(
    std::optional<CredentialCacheEntry> credential,
    GenerationCompletedCallback callback,
    APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  // Handle successful request
  if (success) {
    std::string completion = "";
    // We're checking for a value body in case for non-streaming API results.
    if (result.value_body().is_dict()) {
      const std::string* value =
          result.value_body().GetDict().FindString("completion");
      if (value) {
        // Trimming necessary for Llama 2 which prepends responses with a " ".
        completion = base::TrimWhitespaceASCII(*value, base::TRIM_ALL);
      }
    }

    std::move(callback).Run(base::ok(std::move(completion)));
    return;
  }

  // If error code is not 401, put credential in cache
  if (result.response_code() != net::HTTP_UNAUTHORIZED && credential) {
    credential_manager_->PutCredentialInCache(std::move(*credential));
  }

  // Handle error
  mojom::APIError error;

  if (net::HTTP_TOO_MANY_REQUESTS == result.response_code()) {
    error = mojom::APIError::RateLimitReached;
  } else if (net::HTTP_REQUEST_ENTITY_TOO_LARGE == result.response_code()) {
    error = mojom::APIError::ContextLimitReached;
  } else {
    error = mojom::APIError::ConnectionIssue;
  }

  std::move(callback).Run(base::unexpected(std::move(error)));
}

}  // namespace ai_chat

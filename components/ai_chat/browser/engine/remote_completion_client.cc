/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/engine/remote_completion_client.h"

#include <base/containers/flat_map.h>

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/ai_chat/browser/constants.h"
#include "brave/components/ai_chat/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/common/features.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace ai_chat {
namespace {

constexpr char kAIChatCompletionPath[] = "v1/complete";

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
    const std::vector<std::string> additional_stop_sequences,
    const bool is_sse_enabled) {
  base::Value::Dict dict;

  base::Value::List stop_sequences;
  stop_sequences.Append(RemoteCompletionClient::GetHumanPromptSegment());
  for (auto& item : additional_stop_sequences) {
    stop_sequences.Append(item);
  }

  const double temp = ai_chat::features::kAITemperature.Get();

  DCHECK(!model_name.empty());

  dict.Set("prompt", prompt);
  dict.Set("max_tokens_to_sample", 400);
  dict.Set("temperature", temp);
  dict.Set("top_k", -1);  // disabled
  dict.Set("top_p", 0.999);
  dict.Set("model", model_name);
  dict.Set("stop_sequences", std::move(stop_sequences));
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

const GURL GetEndpointBaseUrl() {
  auto* endpoint = BUILDFLAG(BRAVE_AI_CHAT_ENDPOINT);
  // Simply log if we have empty endpoint, it's probably just a local
  // non-configured build.
  if (strlen(endpoint) != 0) {
    static base::NoDestructor<GURL> url{
        base::StrCat({url::kHttpsScheme, "://", endpoint})};
    return *url;
  } else {
    LOG(ERROR) << "BRAVE_AI_CHAT_ENDPOINT was empty. Must supply an AI Chat"
                  "endpoint via build flag to use the AI Chat feature.";
    return GURL::EmptyGURL();
  }
}

}  // namespace

// static
std::string RemoteCompletionClient::GetHumanPromptSegment() {
  return base::StrCat({"\n\n", kHumanPrompt, " "});
}

RemoteCompletionClient::RemoteCompletionClient(
    std::string model_name,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : model_name_(model_name),
      api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {
  // Validate configuration
  const GURL api_base_url = GetEndpointBaseUrl();
  if (!api_base_url.is_empty()) {
    // Crash quickly if we have an invalid non-empty Url configured
    // as a build flag
    CHECK(api_base_url.is_valid()) << "API Url generated was invalid."
                                      "Please check configuration parameter.";
  }
}

RemoteCompletionClient::~RemoteCompletionClient() = default;

void RemoteCompletionClient::QueryPrompt(
    const std::string& prompt,
    const std::vector<std::string> extra_stop_sequences,
    EngineConsumer::CompletionCompletedCallback data_completed_callback,
    EngineConsumer::CompletionDataReceivedCallback
        data_received_callback /* = base::NullCallback() */) {
  const GURL api_base_url = GetEndpointBaseUrl();

  // Validate that the path is valid
  GURL api_url = api_base_url.Resolve(kAIChatCompletionPath);
  CHECK(api_url.is_valid())
      << "Invalid API Url, check path: " << api_url.spec();

  const bool is_sse_enabled =
      ai_chat::features::kAIChatSSE.Get() && !data_received_callback.is_null();

  const base::Value::Dict& dict = CreateApiParametersDict(
      prompt, model_name_, std::move(extra_stop_sequences), is_sse_enabled);
  base::flat_map<std::string, std::string> headers;
  headers.emplace("x-brave-key", BUILDFLAG(BRAVE_SERVICES_KEY));
  headers.emplace("Accept", "text/event-stream");

  if (is_sse_enabled) {
    VLOG(2) << "Making streaming AI Chat API Request";
    auto on_received = base::BindRepeating(
        &RemoteCompletionClient::OnQueryDataReceived,
        weak_ptr_factory_.GetWeakPtr(), std::move(data_received_callback));
    auto on_complete = base::BindOnce(&RemoteCompletionClient::OnQueryCompleted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(data_completed_callback));

    api_request_helper_.RequestSSE("POST", api_url, CreateJSONRequestBody(dict),
                                   "application/json", std::move(on_received),
                                   std::move(on_complete), headers, {});
  } else {
    VLOG(2) << "Making non-streaming AI Chat API Request";
    auto on_complete = base::BindOnce(&RemoteCompletionClient::OnQueryCompleted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(data_completed_callback));

    api_request_helper_.Request("POST", api_url, CreateJSONRequestBody(dict),
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
    EngineConsumer::CompletionDataReceivedCallback callback,
    base::expected<base::Value, std::string> result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  if (const std::string* completion =
          result->GetDict().FindString("completion")) {
    callback.Run(std::move(*completion));
  }
}

void RemoteCompletionClient::OnQueryCompleted(
    EngineConsumer::CompletionCompletedCallback callback,
    APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  // Handle successful request
  if (success) {
    std::string completion;
    // We're checking for a value body in case for non-streaming API results.
    if (result.value_body().is_dict()) {
      completion = *result.value_body().GetDict().FindString("completion");
      // Trimming necessary for Llama 2 which prepends responses with a " ".
      completion = base::TrimWhitespaceASCII(completion, base::TRIM_ALL);
    } else {
      completion = "";
    }
    std::move(callback).Run(base::ok(std::move(completion)));
    return;
  }
  // Handle error
  mojom::APIError error =
      (net::HTTP_TOO_MANY_REQUESTS == result.response_code())
          ? mojom::APIError::RateLimitReached
          : mojom::APIError::ConnectionIssue;
  std::move(callback).Run(base::unexpected(std::move(error)));
}

}  // namespace ai_chat

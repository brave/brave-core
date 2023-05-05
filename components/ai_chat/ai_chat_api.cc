/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ai_chat_api.h"

#include <base/containers/flat_map.h>

#include <utility>

#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/buildflags.h"
#include "brave/components/ai_chat/constants.h"
#include "brave/components/ai_chat/features.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace {

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

AIChatAPI::AIChatAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
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

AIChatAPI::~AIChatAPI() = default;

void AIChatAPI::QueryPrompt(ResponseCallback callback,
                            const std::string& prompt) {
  const GURL api_base_url = GetEndpointBaseUrl();
  // Verify that we have a Url configured
  if (api_base_url.is_empty()) {
    std::move(callback).Run("", false);
    return;
  }
  // Validate that the path is valid
  GURL api_url = api_base_url.Resolve(ai_chat::kAIChatCompletionPath);
  CHECK(api_url.is_valid())
      << "Invalid API Url, check path: " << api_url.spec();

  auto internal_callback =
      base::BindOnce(&AIChatAPI::OnGetResponse, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback));

  base::Value::Dict dict;
  base::Value::List stop_sequences;
  stop_sequences.Append("\n\nHuman:");

  const auto model_name = ai_chat::features::kAIModelName.Get();
  DCHECK(!model_name.empty());

  dict.Set("prompt", prompt);
  dict.Set("max_tokens_to_sample", 400);
  dict.Set("temperature", 1);
  dict.Set("top_k", -1);  // disabled
  dict.Set("top_p", 0.999);
  dict.Set("model", model_name);
  dict.Set("stop_sequences", std::move(stop_sequences));
  dict.Set("stream", false);

  base::flat_map<std::string, std::string> headers;
  headers.emplace("x-brave-key", BUILDFLAG(BRAVE_SERVICES_KEY));

  DVLOG(1) << __func__ << " Prompt: |" << prompt << "|\n";
  DVLOG(1) << __func__ << " Using model: " << model_name;

  api_request_helper_.Request("POST", api_url, CreateJSONRequestBody(dict),
                              "application/json", std::move(internal_callback),
                              headers);

  DVLOG(1) << __func__ << " API Request sent\n";
}

void AIChatAPI::OnGetResponse(ResponseCallback callback,
                              api_request_helper::APIRequestResult result) {
  const bool success = result.response_code() == 200;

  if (!success) {
    DVLOG(1) << __func__ << " Response from API was not HTTP 200 (Received "
             << result.response_code() << ")";
  }

  std::string response = result.body();
  const base::Value::Dict* dict = result.value_body().GetIfDict();

  if (dict) {
    if (const std::string* completion = dict->FindString("completion")) {
      response = *completion;
    }
  } else {
    DVLOG(1) << __func__ << " Result dict not found\n";
  }

  std::move(callback).Run(response, success);
}

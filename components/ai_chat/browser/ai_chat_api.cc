/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_api.h"

#include <base/containers/flat_map.h>

#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/browser/constants.h"
#include "brave/components/ai_chat/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/common/features.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
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

base::Value::Dict CreateApiParametersDict(
    const std::string& prompt,
    const std::vector<std::string> additional_stop_sequences,
    const bool is_sse_enabled) {
  base::Value::Dict dict;

  base::Value::List stop_sequences;
  stop_sequences.Append(ai_chat::GetHumanPromptSegment());
  for (auto& item : additional_stop_sequences) {
    stop_sequences.Append(item);
  }

  const auto model_name = ai_chat::features::kAIModelName.Get();
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

void AIChatAPI::QueryPrompt(
    const std::string& prompt,
    const std::vector<std::string> extra_stop_sequences,
    api_request_helper::APIRequestHelper::ResultCallback
        data_completed_callback,
    api_request_helper::APIRequestHelper::DataReceivedCallback
        data_received_callback) {
  if (!ai_chat::UsesLlama2PromptTemplate(
          ai_chat::features::kAIModelName.Get())) {
    // All queries must have the "Human" and "AI" prompt markers. We do not
    // prepend / append them here since callers may want to put them in
    // custom positions.
    DCHECK(base::MatchPattern(prompt,
                              base::StrCat({"*", ai_chat::kHumanPrompt, "*"})));
    DCHECK(base::MatchPattern(prompt,
                              base::StrCat({"*", ai_chat::kAIPrompt, "*"})));
  }

  const GURL api_base_url = GetEndpointBaseUrl();

  // Validate that the path is valid
  GURL api_url = api_base_url.Resolve(ai_chat::kAIChatCompletionPath);
  CHECK(api_url.is_valid())
      << "Invalid API Url, check path: " << api_url.spec();

  const bool is_sse_enabled =
      ai_chat::features::kAIChatSSE.Get() && !data_received_callback.is_null();

  const base::Value::Dict& dict = CreateApiParametersDict(
      prompt, std::move(extra_stop_sequences), is_sse_enabled);
  base::flat_map<std::string, std::string> headers;
  headers.emplace("x-brave-key", BUILDFLAG(BRAVE_SERVICES_KEY));
  headers.emplace("Accept", "text/event-stream");

  if (is_sse_enabled) {
    VLOG(2) << "Making streaming AI Chat API Request";
    api_request_helper_.RequestSSE(
        "POST", api_url, CreateJSONRequestBody(dict), "application/json",
        std::move(data_received_callback), std::move(data_completed_callback),
        headers, {});
  } else {
    VLOG(2) << "Making non-streaming AI Chat API Request";
    auto on_result_cb = base::BindOnce(
        [](api_request_helper::APIRequestHelper::ResultCallback
               data_completed_callback,
           api_request_helper::APIRequestResult result) {
          VLOG(2) << "Non-streaming response code: " << result.response_code();
          std::move(data_completed_callback).Run(std::move(result));
        },
        std::move(data_completed_callback));

    api_request_helper_.Request("POST", api_url, CreateJSONRequestBody(dict),
                                "application/json", std::move(on_result_cb),
                                headers, {});
  }
}

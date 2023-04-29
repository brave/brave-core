/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ai_chat/ai_chat_api.h"

#include <base/containers/flat_map.h>

#include <utility>

#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/buildflags.h"
#include "brave/browser/ai_chat/constants.h"
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

GURL GetURLWithPath(const std::string& host, const std::string& path) {
  return GURL(std::string(url::kHttpsScheme) + "://" + host).Resolve(path);
}

std::string CreateJSONRequestBody(base::ValueView node) {
  std::string json;
  base::JSONWriter::Write(node, &json);
  return json;
}

}  // namespace

AIChatAPI::AIChatAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

AIChatAPI::~AIChatAPI() = default;

void AIChatAPI::QueryPrompt(ResponseCallback callback,
                            const std::string& prompt) {
  auto internal_callback =
      base::BindOnce(&AIChatAPI::OnGetResponse, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback));

  base::Value::Dict dict;
  base::Value::List stop_sequences;
  stop_sequences.Append("\n\nHuman:");

  dict.Set("prompt", prompt);
  dict.Set("max_tokens_to_sample", 200);
  dict.Set("temperature", 0.7);
  dict.Set("top_k", -1);  // disabled
  dict.Set("top_p", 0.999);
  dict.Set("model", "claude-v1");
  dict.Set("stop_sequences", std::move(stop_sequences));
  dict.Set("stream", false);

  base::flat_map<std::string, std::string> headers;
  headers.emplace("x-brave-key", BUILDFLAG(BRAVE_SERVICES_KEY));

  VLOG(1) << __func__ << " Prompt: " << prompt << "\n";

  api_request_helper_.Request("POST",
                              GetURLWithPath(BUILDFLAG(BRAVE_AI_CHAT_ENDPOINT),
                                             ai_chat::kAIChatCompletionPath),
                              CreateJSONRequestBody(dict), "application/json",
                              true, std::move(internal_callback), headers);

  VLOG(1) << __func__ << " API Request sent\n";
}

void AIChatAPI::OnGetResponse(ResponseCallback callback,
                              api_request_helper::APIRequestResult result) {
  const bool success = result.response_code() == 200;

  if (!success) {
    VLOG(1) << __func__ << " Response from API was not HTTP 200 (Received "
            << result.response_code() << ")";
    return;
  }

  std::string response = result.body();
  const base::Value::Dict* dict = result.value_body().GetIfDict();

  if (!dict) {
    VLOG(1) << __func__ << " Result dict not found\n";
    return;
  }

  const std::string* completion = dict->FindString("completion");

  if (completion) {
    response = *completion;
  }

  std::move(callback).Run(response, success);
}

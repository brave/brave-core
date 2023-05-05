/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ai_chat_api.h"

#include <base/containers/flat_map.h>

#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "brave/components/ai_chat/buildflags.h"
#include "brave/components/ai_chat/constants.h"
#include "brave/components/ai_chat/features.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
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

  // We need our own instance of DataDecoder and remote for JsonParser to ensure
  // ordered processing of stream chunks. Calling the static function creates a
  // new instance of the process for each call, which may result in unordered
  // chunks. Thus, we create a single instance of the parser and reuse it for
  // consecutive calls
  data_decoder_ = std::make_unique<data_decoder::DataDecoder>();
  data_decoder_->GetService()->BindJsonParser(
      json_parser_.BindNewPipeAndPassReceiver());
}

AIChatAPI::~AIChatAPI() = default;

bool AIChatAPI::IsRequestInProgress() {
  return is_request_in_progress_;
}

void AIChatAPI::QueryPrompt(ResponseCallback response_callback,
                            CompletionCallback completion_callback,
                            const std::string& prompt) {
  const GURL api_base_url = GetEndpointBaseUrl();

  // Validate that the path is valid
  GURL api_url = api_base_url.Resolve(ai_chat::kAIChatCompletionPath);
  CHECK(api_url.is_valid())
      << "Invalid API Url, check path: " << api_url.spec();

  const base::Value::Dict& dict = CreateApiParametersDict(prompt);
  base::flat_map<std::string, std::string> headers;
  headers.emplace("x-brave-key", BUILDFLAG(BRAVE_SERVICES_KEY));
  headers.emplace("Accept", "text/event-stream");

  if (IsRequestInProgress()) {
    // TODO(nullhook): Send an event for this so UI can reject further inputs
    DVLOG(1) << __func__ << "There is a request in progress\n";
    return;
  }

  response_callback_ = response_callback;
  completion_callback_ = std::move(completion_callback);

  current_request_ = api_request_helper_.Request(
      "POST", api_url, CreateJSONRequestBody(dict), "application/json", this,
      headers, {},
      base::BindOnce(&AIChatAPI::OnResponseStarted,
                     weak_ptr_factory_.GetWeakPtr()),
      base::BindRepeating(&AIChatAPI::OnDownloadProgress,
                          weak_ptr_factory_.GetWeakPtr()));

  DVLOG(1) << __func__ << " API Request sent\n";
}

base::Value::Dict AIChatAPI::CreateApiParametersDict(
    const std::string& prompt) {
  base::Value::Dict dict;
  base::Value::List stop_sequences;
  stop_sequences.Append("\n\nHuman:");
  stop_sequences.Append("</response>");

  const auto model_name = ai_chat::features::kAIModelName.Get();
  DCHECK(!model_name.empty());

  dict.Set("prompt", prompt);
  dict.Set("max_tokens_to_sample", 400);
  dict.Set("temperature", 1);
  dict.Set("top_k", -1);  // disabled
  dict.Set("top_p", 0.999);
  dict.Set("model", model_name);
  dict.Set("stop_sequences", std::move(stop_sequences));
  dict.Set("stream", true);

  DVLOG(1) << __func__ << " Prompt: |" << prompt << "|\n";
  DVLOG(1) << __func__ << " Using model: " << model_name;

  return dict;
}

void AIChatAPI::OnDataReceived(base::StringPiece string_piece,
                               base::OnceClosure resume) {
  std::vector<std::string> stream_data = base::SplitString(
      string_piece, "\r\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  static constexpr char kDataPrefix[] = "data: {";

  // Binding a remote will guarantee the order of the messages being sent, so we
  // must ensure that it is connected.
  DCHECK(data_decoder_);
  DCHECK(json_parser_.is_connected());

  for (const auto& data : stream_data) {
    if (!base::StartsWith(data, kDataPrefix)) {
      continue;
    }

    std::string json = data.substr(strlen(kDataPrefix) - 1);

    data_decoder_->ParseJson(json,
                             base::BindOnce(&AIChatAPI::OnParseJsonIsolated,
                                            weak_ptr_factory_.GetWeakPtr()));
  }

  std::move(resume).Run();
}

void AIChatAPI::OnComplete(bool success) {
  DCHECK(completion_callback_);
  DCHECK(current_request_->get());

  int response_code = -1;

  if (current_request_->get()->ResponseInfo() &&
      current_request_->get()->ResponseInfo()->headers) {
    response_code =
        current_request_->get()->ResponseInfo()->headers->response_code();
  }

  is_request_in_progress_ = false;
  current_request_->reset(nullptr);
  std::move(completion_callback_).Run(success, response_code);
}

void AIChatAPI::OnRetry(base::OnceClosure start_retry) {
  // Retries are not enabled for these requests.
  NOTREACHED();
}

void AIChatAPI::OnParseJsonIsolated(
    data_decoder::DataDecoder::ValueOrError result) {
  if (!result.has_value()) {
    return;
  }

  if (const std::string* completion = result->FindStringKey("completion")) {
    response_callback_.Run(*completion);
  }
}

void AIChatAPI::OnResponseStarted(
    const GURL& final_url,
    const network::mojom::URLResponseHead& response_head) {
  is_request_in_progress_ = true;
}

void AIChatAPI::OnDownloadProgress(uint64_t current) {
  is_request_in_progress_ = true;
}

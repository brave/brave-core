// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"

#include <ios>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/engine/extended_content_block.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/constants/brave_services_key.h"
#include "components/grit/brave_components_strings.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to communicate with user-provided server url"
          "on behalf of the user interacting with different browser AI"
          "features."
        trigger:
          "Triggered by user interactions such as submitting an AI Chat"
          "conversation message, or requesting a text rewrite."
        data:
          "Conversational messages input by the user as well as associated"
          "content or user text to be rewritten. Can contain PII."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

std::string CreateJSONRequestBody(
    base::Value::List messages,
    const bool is_sse_enabled,
    const mojom::CustomModelOptions& model_options,
    const std::optional<std::vector<std::string>>& stop_sequences) {
  base::Value::Dict dict;

  dict.Set("messages", std::move(messages));
  dict.Set("stream", is_sse_enabled);
  dict.Set("temperature", 0.7);
  dict.Set("model", model_options.model_request_name);

  if (stop_sequences && !stop_sequences->empty()) {
    base::Value::List stop_list;
    for (const auto& sequence : *stop_sequences) {
      stop_list.Append(sequence);
    }
    dict.Set("stop", std::move(stop_list));
  }

  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

}  // namespace

// static
base::Value::List OAIAPIClient::SerializeOAIMessages(
    std::vector<OAIMessage> messages) {
  base::Value::List serialized_messages;
  for (const auto& message : messages) {
    base::Value::Dict message_dict;
    message_dict.Set("role", std::move(message.role));

    base::Value::List content_list;
    for (const auto& extended_content_block : message.content) {
      base::Value::Dict content_block_dict;

      switch (extended_content_block.type) {
        case ExtendedContentBlockType::kText: {
          content_block_dict.Set("type", "text");

          auto* text_content =
              std::get_if<TextContent>(&extended_content_block.data);
          if (!text_content) {
            DVLOG(2) << "Missing text content for text type";
            continue;
          }

          content_block_dict.Set("text", std::move(text_content->text));
          break;
        }

        case ExtendedContentBlockType::kImage: {
          content_block_dict.Set("type", "image_url");

          auto* image_content =
              std::get_if<ImageContent>(&extended_content_block.data);
          if (!image_content) {
            DVLOG(2) << "Missing image content for image_url type";
            continue;
          }

          base::Value::Dict image_url;
          image_url.Set("url", std::move(image_content->image_url.url));
          if (image_content->image_url.detail) {
            image_url.Set("detail",
                          std::move(*image_content->image_url.detail));
          }
          content_block_dict.Set("image_url", std::move(image_url));
          break;
        }

        case ExtendedContentBlockType::kPageExcerpt: {
          content_block_dict.Set("type", "text");
          auto* text_content =
              std::get_if<TextContent>(&extended_content_block.data);
          if (!text_content) {
            DVLOG(2) << "Missing text content for page excerpt type";
            continue;
          }

          content_block_dict.Set(
              "text", l10n_util::GetStringFUTF8(
                          IDS_AI_CHAT_LLAMA2_SELECTED_TEXT_PROMPT_SEGMENT,
                          base::UTF8ToUTF16(text_content->text)));
          break;
        }

        case ExtendedContentBlockType::kParaphrase: {
          content_block_dict.Set("type", "text");
          content_block_dict.Set("text", l10n_util::GetStringUTF8(
                                             IDS_AI_CHAT_QUESTION_PARAPHRASE));
          break;
        }

        case ExtendedContentBlockType::kImprove: {
          content_block_dict.Set("type", "text");
          content_block_dict.Set(
              "text", l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_IMPROVE));
          break;
        }

        case ExtendedContentBlockType::kShorten: {
          content_block_dict.Set("type", "text");
          content_block_dict.Set(
              "text", l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SHORTEN));
          break;
        }

        case ExtendedContentBlockType::kExpand: {
          content_block_dict.Set("type", "text");
          content_block_dict.Set(
              "text", l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_EXPAND));
          break;
        }

        case ExtendedContentBlockType::kChangeTone: {
          content_block_dict.Set("type", "text");
          auto* tone_content =
              std::get_if<ChangeToneContent>(&extended_content_block.data);
          if (!tone_content) {
            DVLOG(2) << "Missing change tone content for change tone type";
            continue;
          }

          content_block_dict.Set("text",
                                 l10n_util::GetStringFUTF8(
                                     IDS_AI_CHAT_QUESTION_CHANGE_TONE_TEMPLATE,
                                     base::UTF8ToUTF16(tone_content->tone)));
          break;
        }
      }

      content_list.Append(std::move(content_block_dict));
    }
    message_dict.Set("content", std::move(content_list));

    serialized_messages.Append(std::move(message_dict));
  }

  return serialized_messages;
}

OAIAPIClient::OAIAPIClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      GetNetworkTrafficAnnotationTag(), url_loader_factory);
}

OAIAPIClient::~OAIAPIClient() = default;

void OAIAPIClient::ClearAllQueries() {
  api_request_helper_->CancelAll();
}

void OAIAPIClient::PerformRequestWithOAIMessages(
    const mojom::CustomModelOptions& model_options,
    std::vector<OAIMessage> messages,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    const std::optional<std::vector<std::string>>& stop_sequences) {
  PerformRequest(model_options, SerializeOAIMessages(std::move(messages)),
                 std::move(data_received_callback),
                 std::move(completed_callback), stop_sequences);
}

void OAIAPIClient::PerformRequest(
    const mojom::CustomModelOptions& model_options,
    base::Value::List messages,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    const std::optional<std::vector<std::string>>& stop_sequences) {
  if (!model_options.endpoint.is_valid()) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  const bool is_sse_enabled =
      ai_chat::features::kAIChatSSE.Get() && !data_received_callback.is_null();
  const std::string request_body = CreateJSONRequestBody(
      std::move(messages), is_sse_enabled, model_options, stop_sequences);
  base::flat_map<std::string, std::string> headers;
  if (!model_options.api_key.empty()) {
    headers.emplace("Authorization",
                    base::StrCat({"Bearer ", model_options.api_key}));
  }

  if (is_sse_enabled) {
    auto on_received = base::BindRepeating(&OAIAPIClient::OnQueryDataReceived,
                                           weak_ptr_factory_.GetWeakPtr(),
                                           std::move(data_received_callback));
    auto on_complete = base::BindOnce(&OAIAPIClient::OnQueryCompleted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(completed_callback));

    api_request_helper_->RequestSSE(net::HttpRequestHeaders::kPostMethod,
                                    model_options.endpoint, request_body,
                                    "application/json", std::move(on_received),
                                    std::move(on_complete), headers, {});
  } else {
    auto on_complete = base::BindOnce(&OAIAPIClient::OnQueryCompleted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(completed_callback));
    api_request_helper_->Request(
        net::HttpRequestHeaders::kPostMethod, model_options.endpoint,
        request_body, "application/json", std::move(on_complete), headers, {});
  }
}

// When called as part of a SSE request, this method will not contain the body.
// Instead, the body is evaluated in chunks via the OnQueryDataReceived method.
// OnQueryCompleted will instead receive superficial data such as the response
// code. As such, during SSE, this method will run the callback with either a
// completion (which could be an empty string), or an error. We aim to provide
// more information to the user/UI when invalid payloads are received. That
// effort is tracked here: https://github.com/brave/brave-browser/issues/43536
void OAIAPIClient::OnQueryCompleted(
    GenerationCompletedCallback callback,
    api_request_helper::APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  // Handle successful request
  if (success) {
    // We're checking for a value body in case for non-streaming API results.
    if (result.value_body().is_dict()) {
      if (auto result_data = ParseOAICompletionResponse(
              result.value_body().GetDict(), nullptr /* model_service */)) {
        std::move(callback).Run(base::ok(std::move(*result_data)));
        return;
      }
    }

    // May be an empty string if part of SSE request, and payload was invalid.
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(""));
    std::move(callback).Run(base::ok(EngineConsumer::GenerationResultData(
        std::move(event), std::nullopt /* model_key */)));
    return;
  }

  // Determine which type of error occurred.
  // https://platform.openai.com/docs/guides/error-codes
  // https://docs.anthropic.com/en/api/errors
  mojom::APIError error;

  switch (result.response_code()) {
    case 401:  // Incorrect API key provided
      error = mojom::APIError::InvalidAPIKey;
      break;
    case 429:  // Rate limit reached or out of credits
      error = mojom::APIError::RateLimitReached;
      break;
    case 529:  // Temporary server overload
      error = mojom::APIError::ServiceOverloaded;
      break;
    default:
      error = mojom::APIError::ConnectionIssue;
      break;
  }

  // Handle error
  std::move(callback).Run(base::unexpected(error));
}

void OAIAPIClient::OnQueryDataReceived(
    GenerationDataCallback callback,
    base::expected<base::Value, std::string> result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  if (auto result_data = ParseOAICompletionResponse(
          result->GetDict(), nullptr /* model_service */)) {
    callback.Run(std::move(*result_data));
  }
}

}  // namespace ai_chat

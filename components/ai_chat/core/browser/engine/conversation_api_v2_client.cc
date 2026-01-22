// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/conversation_api_v2_client.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/l10n/common/locale_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/functional/overload.h"

namespace ai_chat {

namespace {

// https://github.com/brave/aichat/blob/8fc09e023e8674e1069b7c1c30f848c74c4c1154/aichat/serve/open_ai_api.py#L47
constexpr char kRemotePath[] = "v1/chat/completions";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to communicate with Brave's AI Conversation API"
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

std::string_view GetContentBlockTypeString(
    const mojom::ContentBlockPtr& block) {
  static constexpr auto kSimpleRequestTypeMap =
      base::MakeFixedFlatMap<mojom::SimpleRequestType, std::string_view>({
          {mojom::SimpleRequestType::kParaphrase, "brave-request-paraphrase"},
          {mojom::SimpleRequestType::kImprove,
           "brave-request-improve-excerpt-language"},
          {mojom::SimpleRequestType::kShorten, "brave-request-shorten"},
          {mojom::SimpleRequestType::kExpand, "brave-request-expansion"},
          {mojom::SimpleRequestType::kRequestSummary, "brave-request-summary"},
          {mojom::SimpleRequestType::kRequestQuestions,
           "brave-request-questions"},
      });

  static_assert(kSimpleRequestTypeMap.size() ==
                    static_cast<size_t>(mojom::SimpleRequestType::kCount),
                "kSimpleRequestTypeMap must cover all "
                "SimpleRequestType enum values");

  switch (block->which()) {
    case mojom::ContentBlock::Tag::kTextContentBlock:
      return "text";
    case mojom::ContentBlock::Tag::kImageContentBlock:
      return "image_url";
    case mojom::ContentBlock::Tag::kFileContentBlock:
      return "file";
    case mojom::ContentBlock::Tag::kPageExcerptContentBlock:
      return "brave-page-excerpt";
    case mojom::ContentBlock::Tag::kPageTextContentBlock:
      return "brave-page-text";
    case mojom::ContentBlock::Tag::kVideoTranscriptContentBlock:
      return "brave-video-transcript";
    case mojom::ContentBlock::Tag::kRequestTitleContentBlock:
      return "brave-conversation-title";
    case mojom::ContentBlock::Tag::kChangeToneContentBlock:
      return "brave-request-change-tone";
    case mojom::ContentBlock::Tag::kMemoryContentBlock:
      return "brave-user-memory";
    case mojom::ContentBlock::Tag::kSuggestFocusTopicsContentBlock:
      return "brave-suggest-focus-topics";
    case mojom::ContentBlock::Tag::kSuggestFocusTopicsWithEmojiContentBlock:
      return "brave-suggest-focus-topics-emoji";
    case mojom::ContentBlock::Tag::kFilterTabsContentBlock:
      return "brave-filter-tabs";
    case mojom::ContentBlock::Tag::kReduceFocusTopicsContentBlock:
      return "brave-reduce-focus-topics";
    case mojom::ContentBlock::Tag::kSimpleRequestContentBlock: {
      const auto& request = block->get_simple_request_content_block();
      auto it = kSimpleRequestTypeMap.find(request->type);
      return it->second;
    }
  }
}

}  // namespace

// static
base::Value::List ConversationAPIV2Client::SerializeOAIMessages(
    std::vector<OAIMessage> messages) {
  base::Value::List serialized_messages;
  for (auto& message : messages) {
    base::Value::Dict message_dict;

    // Set role
    message_dict.Set("role", std::move(message.role));

    // Content blocks
    base::Value::List content_list;
    for (auto& block : message.content) {
      base::Value::Dict content_block_dict;

      // Set type for all blocks
      content_block_dict.Set("type", GetContentBlockTypeString(block));

      // Set content data based on union tag
      switch (block->which()) {
        case mojom::ContentBlock::Tag::kTextContentBlock:
          content_block_dict.Set("text", block->get_text_content_block()->text);
          break;

        case mojom::ContentBlock::Tag::kPageExcerptContentBlock:
          content_block_dict.Set("text",
                                 block->get_page_excerpt_content_block()->text);
          break;

        case mojom::ContentBlock::Tag::kPageTextContentBlock:
          content_block_dict.Set("text",
                                 block->get_page_text_content_block()->text);
          break;

        case mojom::ContentBlock::Tag::kVideoTranscriptContentBlock:
          content_block_dict.Set(
              "text", block->get_video_transcript_content_block()->text);
          break;

        case mojom::ContentBlock::Tag::kRequestTitleContentBlock:
          content_block_dict.Set(
              "text", block->get_request_title_content_block()->text);
          break;

        case mojom::ContentBlock::Tag::kImageContentBlock: {
          const auto& image = block->get_image_content_block();
          base::Value::Dict image_url;
          image_url.Set("url", image->image_url.spec());
          content_block_dict.Set("image_url", std::move(image_url));
          break;
        }

        case mojom::ContentBlock::Tag::kFileContentBlock: {
          const auto& file = block->get_file_content_block();
          base::Value::Dict file_dict;
          file_dict.Set("filename", file->filename);
          file_dict.Set("file_data", file->file_data.spec());
          content_block_dict.Set("file", std::move(file_dict));
          break;
        }

        case mojom::ContentBlock::Tag::kChangeToneContentBlock: {
          const auto& tone = block->get_change_tone_content_block();
          // Server currently requires the empty text field to be passed.
          content_block_dict.Set("text", tone->text);
          content_block_dict.Set("tone", tone->tone);
          break;
        }

        case mojom::ContentBlock::Tag::kMemoryContentBlock: {
          const auto& memory_block = block->get_memory_content_block();
          base::Value::Dict memory_dict;
          for (const auto& [key, memory_value] : memory_block->memory) {
            if (memory_value->is_string_value()) {
              memory_dict.Set(key, memory_value->get_string_value());
            } else if (memory_value->is_list_value()) {
              base::Value::List list;
              for (const auto& val : memory_value->get_list_value()) {
                list.Append(val);
              }
              memory_dict.Set(key, std::move(list));
            }
          }
          content_block_dict.Set("memory", std::move(memory_dict));
          break;
        }

        case mojom::ContentBlock::Tag::kSuggestFocusTopicsContentBlock:
          content_block_dict.Set(
              "text", block->get_suggest_focus_topics_content_block()->text);
          break;

        case mojom::ContentBlock::Tag::kSuggestFocusTopicsWithEmojiContentBlock:
          content_block_dict.Set(
              "text",
              block->get_suggest_focus_topics_with_emoji_content_block()->text);
          break;

        case mojom::ContentBlock::Tag::kFilterTabsContentBlock: {
          const auto& filter_tabs = block->get_filter_tabs_content_block();
          content_block_dict.Set("text", filter_tabs->text);
          content_block_dict.Set("topic", filter_tabs->topic);
          break;
        }

        case mojom::ContentBlock::Tag::kReduceFocusTopicsContentBlock:
          content_block_dict.Set(
              "text", block->get_reduce_focus_topics_content_block()->text);
          break;

        case mojom::ContentBlock::Tag::kSimpleRequestContentBlock:
          // Server currently requires the empty text field to be passed.
          content_block_dict.Set("text", "");
          break;
      }
      content_list.Append(std::move(content_block_dict));
    }
    message_dict.Set("content", std::move(content_list));

    // Tool calls
    if (!message.tool_calls.empty()) {
      base::Value::List tool_call_dicts;
      for (const auto& tool_event : message.tool_calls) {
        base::Value::Dict tool_call_dict;
        tool_call_dict.Set("id", tool_event->id);
        tool_call_dict.Set("type", "function");

        base::Value::Dict function_dict;
        function_dict.Set("name", tool_event->tool_name);

        function_dict.Set("arguments", tool_event->arguments_json);

        tool_call_dict.Set("function", std::move(function_dict));
        tool_call_dicts.Append(std::move(tool_call_dict));
      }

      message_dict.Set("tool_calls", std::move(tool_call_dicts));
    }

    if (!message.tool_call_id.empty()) {
      message_dict.Set("tool_call_id", message.tool_call_id);
    }

    serialized_messages.Append(std::move(message_dict));
  }

  return serialized_messages;
}

ConversationAPIV2Client::ConversationAPIV2Client(
    const std::string& model_name,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager,
    ModelService* model_service)
    : model_name_(model_name),
      credential_manager_(credential_manager),
      model_service_(model_service) {
  CHECK(!model_name_.empty());
  api_request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      GetNetworkTrafficAnnotationTag(), url_loader_factory);
}

ConversationAPIV2Client::~ConversationAPIV2Client() = default;

void ConversationAPIV2Client::ClearAllQueries() {
  api_request_helper_->CancelAll();
}

void ConversationAPIV2Client::PerformRequest(
    std::vector<OAIMessage> messages,
    std::optional<base::Value::List> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    mojom::ConversationCapability conversation_capability,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    const std::optional<std::string>& model_name) {
  // Get credentials and then perform request
  auto callback = base::BindOnce(
      &ConversationAPIV2Client::PerformRequestWithCredentials,
      weak_ptr_factory_.GetWeakPtr(), std::move(messages),
      std::move(oai_tool_definitions), preferred_tool_name,
      conversation_capability, model_name, std::move(data_received_callback),
      std::move(completed_callback));
  credential_manager_->FetchPremiumCredential(std::move(callback));
}

std::string ConversationAPIV2Client::CreateJSONRequestBody(
    std::vector<OAIMessage> messages,
    std::optional<base::Value::List> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    mojom::ConversationCapability conversation_capability,
    const std::optional<std::string>& model_name,
    const bool is_sse_enabled) {
  base::Value::Dict dict;

  dict.Set("messages", SerializeOAIMessages(std::move(messages)));

  // Currently server only expects we pass content_agent capability.
  if (conversation_capability == mojom::ConversationCapability::CONTENT_AGENT) {
    dict.Set("brave_capability", "content_agent");
  }
  dict.Set("model", model_name ? *model_name : model_name_);
  dict.Set("system_language",
           base::StrCat({brave_l10n::GetDefaultISOLanguageCodeString(), "_",
                         brave_l10n::GetDefaultISOCountryCodeString()}));
  dict.Set("stream", is_sse_enabled);

  if (oai_tool_definitions.has_value() && !oai_tool_definitions->empty()) {
    dict.Set("tools", std::move(oai_tool_definitions.value()));
  }

  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

void ConversationAPIV2Client::PerformRequestWithCredentials(
    std::vector<OAIMessage> messages,
    std::optional<base::Value::List> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    mojom::ConversationCapability conversation_capability,
    const std::optional<std::string>& model_name,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    std::optional<CredentialCacheEntry> credential) {
  if (messages.empty()) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  bool premium_enabled = credential.has_value();
  const GURL api_url = GetEndpointUrl(premium_enabled, kRemotePath);

  if (!api_url.is_valid()) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  const bool is_sse_enabled =
      ai_chat::features::kAIChatSSE.Get() && !data_received_callback.is_null();
  const std::string request_body = CreateJSONRequestBody(
      std::move(messages), std::move(oai_tool_definitions), preferred_tool_name,
      conversation_capability, model_name, is_sse_enabled);

  base::flat_map<std::string, std::string> headers;
  const auto digest_header = brave_service_keys::GetDigestHeader(request_body);
  headers.emplace(digest_header.first, digest_header.second);
  auto result = brave_service_keys::GetAuthorizationHeader(
      BUILDFLAG(SERVICE_KEY_AICHAT), headers, api_url,
      net::HttpRequestHeaders::kPostMethod, {"digest"});
  headers.emplace(result.first, result.second);

  if (premium_enabled) {
    // Add Leo premium SKU credential as a Cookie header.
    std::string cookie_header_value =
        "__Secure-sku#brave-leo-premium=" + credential->credential;
    headers.emplace("Cookie", cookie_header_value);
  }
  headers.emplace("x-brave-key", BUILDFLAG(BRAVE_SERVICES_KEY));
  headers.emplace("Accept", "text/event-stream");

  if (is_sse_enabled) {
    DVLOG(2) << "Making streaming AI Chat Conversation API Request";
    auto on_received = base::BindRepeating(
        &ConversationAPIV2Client::OnQueryDataReceived,
        weak_ptr_factory_.GetWeakPtr(), std::move(data_received_callback));
    auto on_complete =
        base::BindOnce(&ConversationAPIV2Client::OnQueryCompleted,
                       weak_ptr_factory_.GetWeakPtr(), std::move(credential),
                       std::move(completed_callback));

    api_request_helper_->RequestSSE(net::HttpRequestHeaders::kPostMethod,
                                    api_url, request_body, "application/json",
                                    std::move(on_received),
                                    std::move(on_complete), headers, {});
  } else {
    DVLOG(2) << "Making non-streaming AI Chat Conversation API Request";
    auto on_complete =
        base::BindOnce(&ConversationAPIV2Client::OnQueryCompleted,
                       weak_ptr_factory_.GetWeakPtr(), std::move(credential),
                       std::move(completed_callback));

    api_request_helper_->Request(net::HttpRequestHeaders::kPostMethod, api_url,
                                 request_body, "application/json",
                                 std::move(on_complete), headers, {});
  }
}

void ConversationAPIV2Client::OnQueryCompleted(
    std::optional<CredentialCacheEntry> credential,
    GenerationCompletedCallback callback,
    api_request_helper::APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  // Handle successful request
  if (success) {
    std::optional<bool> is_near_verified = std::nullopt;
    const auto& headers = result.headers();
    if (const auto* header_value =
            base::FindOrNull(headers, kBraveNearVerifiedHeader)) {
      is_near_verified = *header_value == "true";
    }

    // Parse OAI-format response for non-streaming API results
    if (result.value_body().is_dict()) {
      if (auto parsed_result = ParseOAICompletionResponse(
              result.value_body().GetDict(), model_service_)) {
        parsed_result->is_near_verified = is_near_verified;
        std::move(callback).Run(base::ok(std::move(*parsed_result)));
        return;
      }
    }

    // Return null event if no completion was provided in response body, can
    // happen when server send them all via OnQueryDataReceived.
    std::move(callback).Run(
        GenerationResultData{nullptr, std::nullopt, is_near_verified});
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

void ConversationAPIV2Client::OnQueryDataReceived(
    GenerationDataCallback callback,
    base::expected<base::Value, std::string> result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  auto& result_params = result->GetDict();
  if (auto result_data =
          ParseOAICompletionResponse(result_params, model_service_)) {
    callback.Run(std::move(*result_data));
  }

  // Tool calls - in OpenAI format they're inside choices[0].delta.tool_calls
  // or choices[0].message.tool_calls
  const base::Value::Dict* content_container =
      GetOAIContentContainer(result_params);
  if (content_container) {
    if (const base::Value::List* tool_calls =
            content_container->FindList("tool_calls")) {
      // Provide any valid tool use events to the callback
      // ToolUseEventFromToolCallsResponse handles per-tool alignment_check
      for (auto& tool_use_event :
           ToolUseEventFromToolCallsResponse(tool_calls)) {
        auto tool_event = mojom::ConversationEntryEvent::NewToolUseEvent(
            std::move(tool_use_event));
        callback.Run(GenerationResultData(std::move(tool_event), std::nullopt));
      }
    }
  }
}

}  // namespace ai_chat

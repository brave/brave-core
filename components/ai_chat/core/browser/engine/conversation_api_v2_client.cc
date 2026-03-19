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
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/deep_research_parsing.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"
#include "brave/components/ai_chat/core/browser/engine/oai_serialization_utils.h"
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
    case mojom::ContentBlock::Tag::kWebSourcesContentBlock:
      return "brave-chat.webSources";
  }
}

}  // namespace

// static
base::ListValue ConversationAPIV2Client::SerializeOAIMessages(
    std::vector<OAIMessage> messages,
    const E2EEProcessor::EncryptCallback& encrypt_callback) {
  base::ListValue serialized_messages;
  for (auto& message : messages) {
    base::DictValue message_dict;

    // Set role
    message_dict.Set("role", std::move(message.role));

    // Content blocks
    base::ListValue content_list;
    for (auto& block : message.content) {
      base::DictValue content_block_dict;

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
          content_block_dict.Set(
              "image_url",
              ImageContentBlockToDict(*block->get_image_content_block()));
          break;
        }

        case mojom::ContentBlock::Tag::kFileContentBlock: {
          content_block_dict.Set(
              "file", FileContentBlockToDict(*block->get_file_content_block()));
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
          content_block_dict.Set(
              "memory",
              MemoryContentBlockToDict(*block->get_memory_content_block()));
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

        case mojom::ContentBlock::Tag::kWebSourcesContentBlock: {
          auto& web_sources = block->get_web_sources_content_block();
          base::ListValue sources_list;
          for (auto& source : web_sources->sources) {
            base::DictValue source_dict;
            source_dict.Set("title", source->title);
            source_dict.Set("url", source->url.spec());
            source_dict.Set("favicon", source->favicon_url.spec());
            if (source->page_content) {
              source_dict.Set("page_content", std::move(*source->page_content));
            }
            if (source->extra_snippets) {
              base::ListValue snippets_list;
              for (auto& snippet : source->extra_snippets.value()) {
                snippets_list.Append(std::move(snippet));
              }
              source_dict.Set("extra_snippets", std::move(snippets_list));
            }
            sources_list.Append(std::move(source_dict));
          }
          content_block_dict.Set("sources", std::move(sources_list));
          if (!web_sources->queries.empty()) {
            if (web_sources->queries.size() == 1) {
              content_block_dict.Set("query", web_sources->queries.front());
            } else {
              base::ListValue queries_list;
              for (const auto& q : web_sources->queries) {
                queries_list.Append(q);
              }
              content_block_dict.Set("query", std::move(queries_list));
            }
          }
          if (!web_sources->rich_results.empty()) {
            base::ListValue rich_results_list;
            for (const auto& rich_result : web_sources->rich_results) {
              auto parsed =
                  base::JSONReader::Read(rich_result, base::JSON_PARSE_RFC);
              if (parsed.has_value()) {
                rich_results_list.Append(std::move(*parsed));
              }
            }
            content_block_dict.Set("rich_results",
                                   std::move(rich_results_list));
          }
          break;
        }
      }
      content_list.Append(std::move(content_block_dict));
    }

    if (encrypt_callback) {
      std::optional<std::string> encrypted = encrypt_callback.Run(content_list);
      if (encrypted) {
        message_dict.Set("content", std::move(*encrypted));
      }
    } else {
      message_dict.Set("content", std::move(content_list));
    }

    SerializeToolCallsOnMessageDict(message, message_dict);

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
      model_service_(model_service),
      url_loader_factory_(url_loader_factory) {
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
    std::optional<base::ListValue> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    const ConversationCapabilitySet& conversation_capabilities,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    const std::optional<std::string>& model_name) {
  std::optional<std::string> attestation_model_name;
  if (conversation_capabilities.contains(
          mojom::ConversationCapability::ENCRYPTION)) {
    attestation_model_name = model_name.value_or(model_name_);
  }

  auto callback = base::BindOnce(
      &ConversationAPIV2Client::PerformRequestWithCredentials,
      weak_ptr_factory_.GetWeakPtr(), std::move(messages),
      std::move(oai_tool_definitions), preferred_tool_name,
      conversation_capabilities, model_name, std::move(data_received_callback),
      std::move(completed_callback));

  // Get credentials and then perform request.
  credential_manager_->FetchPremiumCredential(
      base::BindOnce(&ConversationAPIV2Client::OnCredentialFetched,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(attestation_model_name), std::move(callback)));
}

void ConversationAPIV2Client::OnCredentialFetched(
    const std::optional<std::string>& attestation_model_name,
    base::OnceCallback<void(std::optional<CredentialCacheEntry>,
                            std::optional<mojom::APIError>)> callback,
    std::optional<CredentialCacheEntry> credential) {
  if (attestation_model_name.has_value()) {
    EnsureE2EEProcessor();
    e2ee_processor_->FetchModelAttestation(
        *attestation_model_name,
        base::BindOnce(std::move(callback), std::move(credential)));
  } else {
    std::move(callback).Run(std::move(credential), std::nullopt);
  }
}

std::string ConversationAPIV2Client::CreateJSONRequestBody(
    std::vector<OAIMessage> messages,
    std::optional<base::ListValue> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    const ConversationCapabilitySet& conversation_capabilities,
    const std::optional<std::string>& model_name,
    const bool is_sse_enabled,
    const std::optional<std::string>& client_public_key_hex) {
  base::DictValue dict;

  E2EEProcessor::EncryptCallback encrypt_callback;
  if (client_public_key_hex) {
    CHECK(e2ee_processor_);
    encrypt_callback = e2ee_processor_->CreateEncryptCallback(
        model_name.value_or(model_name_));
  }
  dict.Set("messages",
           SerializeOAIMessages(std::move(messages), encrypt_callback));

  base::ListValue capabilities_list;
  for (const auto& capability : conversation_capabilities) {
    const auto* capability_str =
        base::FindOrNull(kCapabilityStringMap, capability);
    CHECK(capability_str) << "Missing string for capability: " << capability;
    capabilities_list.Append(*capability_str);
  }
  dict.Set("brave_capability", std::move(capabilities_list));
  dict.Set("model", model_name ? *model_name : model_name_);
  dict.Set("system_language",
           base::StrCat({brave_l10n::GetDefaultISOLanguageCodeString(), "_",
                         brave_l10n::GetDefaultISOCountryCodeString()}));
  dict.Set("stream", is_sse_enabled);

  if (oai_tool_definitions.has_value() && !oai_tool_definitions->empty()) {
    dict.Set("tools", std::move(oai_tool_definitions.value()));
  }

  if (client_public_key_hex.has_value()) {
    dict.Set("brave_client_public_key", *client_public_key_hex);
  }

  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

void ConversationAPIV2Client::EnsureE2EEProcessor() {
  if (!e2ee_processor_) {
    e2ee_processor_ = std::make_unique<E2EEProcessor>(url_loader_factory_);
  }
}

void ConversationAPIV2Client::PerformRequestWithCredentials(
    std::vector<OAIMessage> messages,
    std::optional<base::ListValue> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    const ConversationCapabilitySet& conversation_capabilities,
    const std::optional<std::string>& model_name,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    std::optional<CredentialCacheEntry> credential,
    std::optional<mojom::APIError> attestation_fetch_error) {
  if (messages.empty()) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  if (attestation_fetch_error.has_value()) {
    std::move(completed_callback)
        .Run(base::unexpected(*attestation_fetch_error));
    return;
  }

  bool premium_enabled = credential.has_value();
  const GURL api_url = GetEndpointUrl(premium_enabled, kRemotePath);

  if (!api_url.is_valid()) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  // Generate a client keypair if encryption is active for this request.
  std::optional<E2EEProcessor::ClientSecretKeyBox> secret_key;
  std::optional<std::string> client_public_key_hex;
  E2EEProcessor::DecryptCallback decrypt_callback;
  if (conversation_capabilities.contains(
          mojom::ConversationCapability::ENCRYPTION)) {
    EnsureE2EEProcessor();
    auto keypair = e2ee_processor_->GenerateClientKeyPair();
    client_public_key_hex = keypair.public_key_hex;
    secret_key = std::move(keypair.secret_key);
    decrypt_callback = e2ee_processor_->CreateDecryptCallback(&**secret_key);
  }

  const bool is_sse_enabled =
      ai_chat::features::kAIChatSSE.Get() && !data_received_callback.is_null();
  const std::string request_body = CreateJSONRequestBody(
      std::move(messages), std::move(oai_tool_definitions), preferred_tool_name,
      conversation_capabilities, model_name, is_sse_enabled,
      client_public_key_hex);

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
    auto on_received =
        base::BindRepeating(&ConversationAPIV2Client::OnQueryDataReceived,
                            weak_ptr_factory_.GetWeakPtr(), decrypt_callback,
                            std::move(data_received_callback));
    auto on_complete =
        base::BindOnce(&ConversationAPIV2Client::OnQueryCompleted,
                       weak_ptr_factory_.GetWeakPtr(), std::move(credential),
                       std::move(secret_key), std::move(completed_callback));

    api_request_helper_->RequestSSE(net::HttpRequestHeaders::kPostMethod,
                                    api_url, request_body, "application/json",
                                    std::move(on_received),
                                    std::move(on_complete), headers, {});
  } else {
    DVLOG(2) << "Making non-streaming AI Chat Conversation API Request";
    auto on_complete =
        base::BindOnce(&ConversationAPIV2Client::OnQueryCompleted,
                       weak_ptr_factory_.GetWeakPtr(), std::move(credential),
                       std::move(secret_key), std::move(completed_callback));

    api_request_helper_->Request(net::HttpRequestHeaders::kPostMethod, api_url,
                                 request_body, "application/json",
                                 std::move(on_complete), headers, {});
  }
}

void ConversationAPIV2Client::OnQueryCompleted(
    std::optional<CredentialCacheEntry> credential,
    std::optional<E2EEProcessor::ClientSecretKeyBox> secret_key,
    GenerationCompletedCallback callback,
    api_request_helper::APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  // Handle successful request
  if (success) {
    std::optional<bool> is_near_verified = std::nullopt;
    std::optional<std::string> model_key = std::nullopt;
    const auto& headers = result.headers();
    if (const auto* header_value =
            base::FindOrNull(headers, kBraveNearVerifiedHeader)) {
      is_near_verified = *header_value == "true";
    }

    // Parse OAI-format response for non-streaming API results
    if (result.value_body().is_dict()) {
      auto& result_dict = result.value_body().GetDict();
      model_key = GetLeoModelKeyFromResponse(result_dict);
      E2EEProcessor::DecryptCallback decrypt_callback;
      if (secret_key) {
        CHECK(e2ee_processor_);
        decrypt_callback =
            e2ee_processor_->CreateDecryptCallback(&**secret_key);
      }
      if (auto parsed_result = ParseOAICompletionResponse(
              result_dict, model_key, decrypt_callback)) {
        parsed_result->is_near_verified = is_near_verified;
        // secret_key box is destroyed here, releasing the key.
        std::move(callback).Run(base::ok(std::move(*parsed_result)));
        return;
      }
    }

    // Return null event if no completion was provided in response body, can
    // happen when server send them all via OnQueryDataReceived.
    // secret_key box is destroyed here, releasing the key.
    std::move(callback).Run(
        GenerationResultData{nullptr, std::move(model_key), is_near_verified});
    return;
  }

  if (secret_key) {
    CHECK(e2ee_processor_);
    // Clear the cached attestation in case a stale public key caused the
    // failure (e.g. server-side key rotation), forcing a fresh fetch on the
    // next request.
    e2ee_processor_->ClearCachedModelAttestations();
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
    E2EEProcessor::DecryptCallback decrypt_callback,
    GenerationDataCallback callback,
    base::expected<base::Value, std::string> result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  auto& result_params = result->GetDict();
  std::optional<std::string> model_key =
      GetLeoModelKeyFromResponse(result_params);
  const auto* object_type = result_params.FindString("object");
  if (!object_type) {
    return;
  }

  if (*object_type == "chat.completion.chunk") {
    if (auto result_data = ParseOAICompletionResponse(result_params, model_key,
                                                      decrypt_callback)) {
      callback.Run(std::move(*result_data));
    }
  } else if (*object_type == "brave-chat.contentReceipt") {
    uint64_t total_tokens = base::saturated_cast<uint64_t>(
        result_params.FindInt("total_tokens").value_or(0));
    uint64_t trimmed_tokens = base::saturated_cast<uint64_t>(
        result_params.FindInt("trimmed_tokens").value_or(0));
    auto event = mojom::ConversationEntryEvent::NewContentReceiptEvent(
        mojom::ContentReceiptEvent::New(total_tokens, trimmed_tokens));

    callback.Run(GenerationResultData(std::move(event), model_key));
  } else if (*object_type == "brave-chat.toolStart") {
    const std::string* tool_name = result_params.FindString("tool_name");
    if (tool_name && IsBraveSearchTool(*tool_name)) {
      auto event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
          mojom::SearchStatusEvent::New(true));
      callback.Run(GenerationResultData(std::move(event), std::nullopt));
    }
  } else if (*object_type == "brave-chat.inlineSearch") {
    auto* query = result_params.FindString("query");
    auto* results = result_params.FindList("results");
    if (query && !query->empty() && results) {
      std::string results_json;
      base::JSONWriter::Write(*results, &results_json);
      auto event = mojom::ConversationEntryEvent::NewInlineSearchEvent(
          mojom::InlineSearchEvent::New(*query, std::move(results_json)));
      callback.Run(GenerationResultData(std::move(event), model_key));
    }
  } else if (base::StartsWith(*object_type, "brave-chat.deepResearch")) {
    if (auto event = ParseDeepResearchEvent(*object_type, result_params)) {
      callback.Run(GenerationResultData(std::move(event), model_key));
    }
  }

  // Tool calls - in OpenAI format they're inside choices[0].delta.tool_calls
  // or choices[0].message.tool_calls
  for (auto& tool_result :
       ParseToolCallsFromOAIResponse(result_params, model_key)) {
    callback.Run(std::move(tool_result));
  }
}

std::optional<std::string> ConversationAPIV2Client::GetLeoModelKeyFromResponse(
    const base::DictValue& response) {
  const std::string* model = response.FindString("model");
  if (!model_service_ || !model) {
    return std::nullopt;
  }

  return model_service_->GetLeoModelKeyByName(*model);
}

}  // namespace ai_chat

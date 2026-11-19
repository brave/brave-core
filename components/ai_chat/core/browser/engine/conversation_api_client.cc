// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
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
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/api_request_helper/sse_parser.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/endpoint_client/client.h"
#include "brave/components/endpoint_client/request_handle.h"
#include "brave/components/endpoint_client/url_replacements.h"
#include "brave/components/l10n/common/locale_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

namespace {

// Error type returned with a 429 when the premium rate limit for the specific
// model has been reached.
constexpr char kModelRateLimitErrorType[] = "42904";

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

// Replaces every component of an endpoint's URL with those of `url`.
endpoint_client::UrlReplacements UrlReplacementsFor(const GURL& url) {
  endpoint_client::UrlReplacements url_replacements;
  url_replacements.SetScheme(std::string(url.scheme()))
      .SetHost(std::string(url.host()))
      .SetPort(std::string(url.port()))
      .SetPath(std::string(url.path()))
      .SetQuery(std::string(url.query()));
  return url_replacements;
}

std::optional<bool> IsNearVerified(const net::HttpResponseHeaders* headers) {
  if (!headers) {
    return std::nullopt;
  }

  return headers->GetNormalizedHeader(kBraveNearVerifiedHeader)
      .transform([](const std::string& value) { return value == "true"; });
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
    case mojom::ContentBlock::Tag::kFileExtractedTextContentBlock:
      return "brave-file-extracted-text";
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

const base::DictValue* GetErrorDict(const base::Value& body) {
  if (!body.is_dict()) {
    return nullptr;
  }
  return body.GetDict().FindDict("error");
}

std::string ParseErrorCode(const base::Value& body) {
  if (const auto* error_dict = GetErrorDict(body)) {
    if (const auto* type_str = error_dict->FindString("type")) {
      return *type_str;
    }
  }
  return std::string();
}

std::optional<base::Time> ParseRateLimitExpiresAt(const base::Value& body) {
  const auto* error_dict = GetErrorDict(body);
  if (!error_dict) {
    return std::nullopt;
  }

  const auto* timestamp_str = error_dict->FindString("rate_limit_expires_at");
  if (!timestamp_str) {
    return std::nullopt;
  }

  base::Time time;
  if (!base::Time::FromUTCString(timestamp_str->c_str(), &time)) {
    return std::nullopt;
  }
  return time;
}

}  // namespace

// static
base::ListValue ConversationAPIClient::SerializeOAIMessages(
    std::vector<OAIMessage> messages) {
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

        case mojom::ContentBlock::Tag::kFileExtractedTextContentBlock:
          content_block_dict.Set(
              "content", block->get_file_extracted_text_content_block()->text);
          break;

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
    message_dict.Set("content", std::move(content_list));

    SerializeToolCallsOnMessageDict(message, message_dict);

    serialized_messages.Append(std::move(message_dict));
  }

  return serialized_messages;
}

struct ConversationAPIClient::Query {
  endpoint_client::RequestHandle request_handle;
  // Only for streaming requests.
  std::unique_ptr<api_request_helper::SSEParser> sse_parser;
  std::optional<bool> is_near_verified;
  // Whether a 2xx response has started, so the credential has been spent.
  bool request_accepted = false;
};

struct ConversationAPIClient::QueryResult {
  // Only set on net::OK.
  std::optional<int> status_code;
  std::optional<bool> is_near_verified;
  // The body of a non-streaming 2xx response, or the error body.
  base::Value body;
  bool request_accepted = false;
};

ConversationAPIClient::ConversationAPIClient(
    const std::string& model_name,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager,
    ModelService* model_service)
    : model_name_(model_name),
      url_loader_factory_(std::move(url_loader_factory)),
      decoder_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      credential_manager_(credential_manager),
      model_service_(model_service) {
  CHECK(!model_name_.empty());
}

ConversationAPIClient::~ConversationAPIClient() = default;

void ConversationAPIClient::ClearAllQueries() {
  queries_.clear();
}

void ConversationAPIClient::PerformRequest(
    std::vector<OAIMessage> messages,
    std::optional<base::ListValue> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    const ConversationCapabilitySet& conversation_capabilities,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    const std::optional<std::string>& model_name) {
  // Get credentials and then perform request
  auto callback = base::BindOnce(
      &ConversationAPIClient::PerformRequestWithCredentials,
      weak_ptr_factory_.GetWeakPtr(), std::move(messages),
      std::move(oai_tool_definitions), preferred_tool_name,
      conversation_capabilities, model_name, std::move(data_received_callback),
      std::move(completed_callback));
  credential_manager_->FetchPremiumCredential(std::move(callback));
}

std::string ConversationAPIClient::CreateJSONRequestBody(
    std::vector<OAIMessage> messages,
    std::optional<base::ListValue> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    const ConversationCapabilitySet& conversation_capabilities,
    const std::optional<std::string>& model_name,
    const bool is_sse_enabled) {
  return CHECK_DEREF(base::WriteJson(CreateRequestBody(
      std::move(messages), std::move(oai_tool_definitions), preferred_tool_name,
      conversation_capabilities, model_name, is_sse_enabled)));
}

base::DictValue ConversationAPIClient::CreateRequestBody(
    std::vector<OAIMessage> messages,
    std::optional<base::ListValue> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    const ConversationCapabilitySet& conversation_capabilities,
    const std::optional<std::string>& model_name,
    const bool is_sse_enabled) {
  base::DictValue dict;

  dict.Set("messages", SerializeOAIMessages(std::move(messages)));

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

  return dict;
}

void ConversationAPIClient::PerformRequestWithCredentials(
    std::vector<OAIMessage> messages,
    std::optional<base::ListValue> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    const ConversationCapabilitySet& conversation_capabilities,
    const std::optional<std::string>& model_name,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    std::optional<CredentialCacheEntry> credential) {
  if (messages.empty()) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  bool premium_enabled = credential.has_value();
  const GURL api_url = GetEndpointUrl(premium_enabled, kConversationAPIPath);

  if (!api_url.is_valid()) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  const bool is_sse_enabled =
      ai_chat::features::kAIChatSSE.Get() && !data_received_callback.is_null();

  endpoint_client::WithHeaders<ConversationAPIEndpoint::Request> request;
  request.network_traffic_annotation_tag =
      net::MutableNetworkTrafficAnnotationTag(GetNetworkTrafficAnnotationTag());
  request.url_replacements = UrlReplacementsFor(api_url);
  request.body.dict = CreateRequestBody(
      std::move(messages), std::move(oai_tool_definitions), preferred_tool_name,
      conversation_capabilities, model_name, is_sse_enabled);

  // The digest has to cover the exact bytes Client<> sends, which it
  // serializes with base::WriteJson() too.
  auto headers = GetBraveHeaders(credential);
  const auto digest_header = brave_service_keys::GetDigestHeader(
      CHECK_DEREF(base::WriteJson(request.body.dict)));
  headers.emplace(digest_header.first, digest_header.second);
  auto auth_header = brave_service_keys::GetAuthorizationHeader(
      BUILDFLAG(SERVICE_KEY_AICHAT), headers, api_url,
      net::HttpRequestHeaders::kPostMethod, {"digest"});
  headers.emplace(auth_header.first, auth_header.second);
  headers.emplace("Accept", "text/event-stream");
  for (const auto& [name, value] : headers) {
    request.headers.SetHeader(name, value);
  }

  const uint64_t query_id = next_query_id_++;
  auto& query =
      *queries_.emplace(query_id, std::make_unique<Query>()).first->second;

  if (is_sse_enabled) {
    DVLOG(2) << "Making streaming AI Chat Conversation API Request";
    query.sse_parser = std::make_unique<api_request_helper::SSEParser>(
        decoder_task_runner_,
        base::BindRepeating(&ConversationAPIClient::OnQueryDataReceived,
                            weak_ptr_factory_.GetWeakPtr(),
                            std::move(data_received_callback)));
    query.request_handle =
        endpoint_client::Client<ConversationAPIStreamEndpoint>::Send(
            *url_loader_factory_, std::move(request),
            {.on_response_started =
                 base::BindOnce(&ConversationAPIClient::OnStreamStarted,
                                weak_ptr_factory_.GetWeakPtr(), query_id),
             .on_chunk =
                 base::BindRepeating(&ConversationAPIClient::OnStreamChunk,
                                     weak_ptr_factory_.GetWeakPtr(), query_id),
             .on_complete =
                 base::BindOnce(&ConversationAPIClient::MakeStreamQueryResult)
                     .Then(base::BindOnce(
                         &ConversationAPIClient::OnStreamCompleted,
                         weak_ptr_factory_.GetWeakPtr(), query_id,
                         std::move(credential),
                         std::move(completed_callback)))});
  } else {
    DVLOG(2) << "Making non-streaming AI Chat Conversation API Request";
    query.request_handle =
        endpoint_client::Client<ConversationAPIEndpoint>::Send<
            endpoint_client::RequestCancelability::kCancelable>(
            *url_loader_factory_, std::move(request),
            base::BindOnce(&ConversationAPIClient::MakeQueryResult)
                .Then(base::BindOnce(&ConversationAPIClient::FinishQuery,
                                     weak_ptr_factory_.GetWeakPtr(), query_id,
                                     std::move(credential),
                                     std::move(completed_callback))));
  }
}

// static
ConversationAPIClient::QueryResult ConversationAPIClient::MakeQueryResult(
    endpoint_client::WithHeaders<ConversationAPIEndpoint::Response> response) {
  QueryResult result;
  result.status_code = response.status_code;
  result.is_near_verified = IsNearVerified(response.headers.get());
  result.request_accepted = response.status_code &&
                            network::IsSuccessfulStatus(*response.status_code);
  if (response.body) {
    result.body = response.body->has_value()
                      ? std::move(response.body->value().value)
                      : std::move(response.body->error().value);
  }
  return result;
}

// static
ConversationAPIClient::QueryResult ConversationAPIClient::MakeStreamQueryResult(
    ConversationAPIStreamEndpoint::Response::Result result) {
  QueryResult query_result;
  query_result.status_code = result.status_code;
  if (result.error_body) {
    query_result.body = std::move(result.error_body->value);
  }
  return query_result;
}

void ConversationAPIClient::OnStreamStarted(
    uint64_t query_id,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  auto* query = base::FindPtrOrNull(queries_, query_id);
  if (!query) {
    return;
  }

  query->is_near_verified = IsNearVerified(headers.get());
  query->request_accepted =
      headers && network::IsSuccessfulStatus(headers->response_code());
}

void ConversationAPIClient::OnStreamChunk(uint64_t query_id,
                                          std::string_view chunk) {
  if (auto* query = base::FindPtrOrNull(queries_, query_id)) {
    query->sse_parser->Process(chunk);
  }
}

void ConversationAPIClient::OnStreamCompleted(
    uint64_t query_id,
    std::optional<CredentialCacheEntry> credential,
    GenerationCompletedCallback callback,
    QueryResult result) {
  auto* query = base::FindPtrOrNull(queries_, query_id);
  if (!query) {
    return;
  }

  result.is_near_verified = query->is_near_verified;
  result.request_accepted = query->request_accepted;

  // The parser parses events on `decoder_task_runner_`, which is sequenced, so
  // this reply runs after those of the events dispatched so far.
  decoder_task_runner_->PostTaskAndReply(
      FROM_HERE, base::DoNothing(),
      base::BindOnce(&ConversationAPIClient::FinishQuery,
                     weak_ptr_factory_.GetWeakPtr(), query_id,
                     std::move(credential), std::move(callback),
                     std::move(result)));
}

void ConversationAPIClient::FinishQuery(
    uint64_t query_id,
    std::optional<CredentialCacheEntry> credential,
    GenerationCompletedCallback callback,
    QueryResult result) {
  if (!queries_.erase(query_id)) {
    return;
  }

  OnQueryCompleted(std::move(credential), std::move(callback),
                   std::move(result));
}

void ConversationAPIClient::OnQueryCompleted(
    std::optional<CredentialCacheEntry> credential,
    GenerationCompletedCallback callback,
    QueryResult result) {
  // Handle successful request
  if (result.status_code && network::IsSuccessfulStatus(*result.status_code)) {
    std::optional<std::string> model_key = std::nullopt;

    // Parse OAI-format response for non-streaming API results
    if (result.body.is_dict()) {
      auto& result_dict = result.body.GetDict();
      model_key = GetLeoModelKeyFromResponse(result_dict);
      if (auto parsed_result =
              ParseOAICompletionResponse(result_dict, model_key)) {
        parsed_result->is_near_verified = result.is_near_verified;
        std::move(callback).Run(base::ok(std::move(*parsed_result)));
        return;
      }
    }

    // Return null event if no completion was provided in response body, can
    // happen when server send them all via OnQueryDataReceived.
    std::move(callback).Run(GenerationResultData{nullptr, std::move(model_key),
                                                 result.is_near_verified});
    return;
  }

  // No status code means the request failed, possibly midway through a stream.
  const int response_code = result.status_code.value_or(-1);

  // If error code is not 401, put credential in cache, unless a 2xx response
  // has already spent it.
  if (response_code != net::HTTP_UNAUTHORIZED && !result.request_accepted &&
      credential) {
    credential_manager_->PutCredentialInCache(std::move(*credential));
  }

  // Handle error
  mojom::APIError error;
  std::string error_type = ParseErrorCode(result.body);
  std::optional<base::Time> rate_limit_expires_at;

  if (net::HTTP_TOO_MANY_REQUESTS == response_code) {
    if (error_type == kModelRateLimitErrorType) {
      error = mojom::APIError::ModelRateLimitReached;
      rate_limit_expires_at = ParseRateLimitExpiresAt(result.body);
    } else {
      error = mojom::APIError::RateLimitReached;
    }
  } else if (net::HTTP_REQUEST_ENTITY_TOO_LARGE == response_code) {
    error = mojom::APIError::ContextLimitReached;
  } else {
    error = mojom::APIError::ConnectionIssue;
  }

  auto details = mojom::APIErrorDetails::New(
      static_cast<int32_t>(response_code), std::move(error_type),
      /*inner_status_code=*/0, rate_limit_expires_at);

  std::move(callback).Run(
      base::unexpected(EngineConsumer::Error(error, std::move(details))));
}

void ConversationAPIClient::OnQueryDataReceived(
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
    if (auto result_data =
            ParseOAICompletionResponse(result_params, model_key)) {
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

std::optional<std::string> ConversationAPIClient::GetLeoModelKeyFromResponse(
    const base::DictValue& response) {
  const std::string* model = response.FindString("model");
  if (!model_service_ || !model) {
    return std::nullopt;
  }

  return model_service_->GetLeoModelKeyByName(*model);
}

}  // namespace ai_chat

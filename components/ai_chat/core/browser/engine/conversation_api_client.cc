// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"

#include <ios>
#include <map>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/command_line.h"
#include "base/containers/checked_iterators.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/field_trial_params.h"
#include "base/no_destructor.h"
#include "base/numerics/clamped_math.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_parsing.h"
#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/l10n/common/locale_util.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

using ConversationEvent = ConversationAPIClient::ConversationEvent;
using ConversationEventType = ConversationAPIClient::ConversationEventType;
using ConversationEventRole = ConversationAPIClient::ConversationEventRole;

constexpr char kRemotePath[] = "v1/conversation";

constexpr char kAllowedWebSourceFaviconHost[] = "imgs.search.brave.com";

#if !defined(OFFICIAL_BUILD)
constexpr char kAIChatServerUrl[] = "ai-chat-server-url";
#endif

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

base::Value::List ConversationEventsToList(
    std::vector<ConversationEvent> conversation) {
  static const base::NoDestructor<std::map<ConversationEventRole, std::string>>
      kRoleMap({{ConversationEventRole::User, "user"},
                {ConversationEventRole::Assistant, "assistant"},
                {ConversationEventRole::Tool, "tool"}});

  static const base::NoDestructor<std::map<ConversationEventType, std::string>>
      kTypeMap(
          {{ConversationEventType::ContextURL, "contextURL"},
           {ConversationEventType::UserText, "userText"},
           {ConversationEventType::PageText, "pageText"},
           {ConversationEventType::PageExcerpt, "pageExcerpt"},
           {ConversationEventType::VideoTranscript, "videoTranscript"},
           {ConversationEventType::VideoTranscriptXML, "videoTranscriptXML"},
           {ConversationEventType::VideoTranscriptVTT, "videoTranscriptVTT"},
           {ConversationEventType::ChatMessage, "chatMessage"},
           {ConversationEventType::RequestRewrite, "requestRewrite"},
           {ConversationEventType::RequestSummary, "requestSummary"},
           {ConversationEventType::RequestSuggestedActions,
            "requestSuggestedActions"},
           {ConversationEventType::SuggestedActions, "suggestedActions"},
           {ConversationEventType::GetSuggestedTopicsForFocusTabs,
            "suggestFocusTopics"},
           {ConversationEventType::DedupeTopics, "dedupeFocusTopics"},
           {ConversationEventType::GetSuggestedAndDedupeTopicsForFocusTabs,
            "suggestAndDedupeFocusTopics"},
           {ConversationEventType::GetFocusTabsForTopic, "classifyTabs"},
           {ConversationEventType::UploadImage, "uploadImage"},
           {ConversationEventType::PageScreenshot, "pageScreenshot"},
           {ConversationEventType::ToolUse, "toolUse"}});

  base::Value::List events;
  for (const auto& event : conversation) {
    base::Value::Dict event_dict;

    // Set role
    auto role_it = kRoleMap->find(event.role);
    CHECK(role_it != kRoleMap->end());
    event_dict.Set("role", role_it->second);

    // Set type
    auto type_it = kTypeMap->find(event.type);
    CHECK(type_it != kTypeMap->end());
    event_dict.Set("type", type_it->second);

    // Content string or content blocks
    event_dict.Set("content", ContentBlocksToJson(event.content));

    // Tool calls
    if (!event.tool_calls.empty()) {
      base::Value::List tool_call_dicts;
      for (const auto& tool_event : event.tool_calls) {
        base::Value::Dict tool_call_dict;
        tool_call_dict.Set("id", tool_event->id);
        tool_call_dict.Set("type", "function");

        base::Value::Dict function_dict;
        function_dict.Set("name", tool_event->tool_name);

        function_dict.Set("arguments", tool_event->arguments_json);

        tool_call_dict.Set("function", std::move(function_dict));
        tool_call_dicts.Append(std::move(tool_call_dict));
      }

      event_dict.Set("tool_calls", std::move(tool_call_dicts));
    }

    if (!event.tool_call_id.empty()) {
      event_dict.Set("tool_call_id", event.tool_call_id);
    }

    if (event.type == ConversationEventType::GetFocusTabsForTopic) {
      event_dict.Set("topic", event.topic);
    }

    events.Append(std::move(event_dict));
  }
  return events;
}

GURL GetEndpointUrl(bool premium, const std::string& path) {
  CHECK(!path.starts_with("/"));

#if !defined(OFFICIAL_BUILD)
  // If a runtime AI Chat URL is provided, use it.
  std::string ai_chat_url =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          kAIChatServerUrl);
  if (!ai_chat_url.empty()) {
    GURL url = GURL(base::StrCat({ai_chat_url, "/", path}));
    CHECK(url.is_valid()) << "Invalid API Url: " << url.spec();
    return url;
  }
#endif

  auto* prefix = premium ? "ai-chat-premium.bsg" : "ai-chat.bsg";
  auto hostname = brave_domains::GetServicesDomain(
      prefix, brave_domains::ServicesEnvironment::DEV);

  GURL url{base::StrCat(
      {url::kHttpsScheme, url::kStandardSchemeSeparator, hostname, "/", path})};

  CHECK(url.is_valid()) << "Invalid API Url: " << url.spec();

  return url;
}

}  // namespace

ConversationAPIClient::ConversationEvent::ConversationEvent(
    ConversationEventRole role,
    ConversationEventType type,
    Content content,
    const std::string& topic,
    std::vector<mojom::ToolUseEventPtr> tool_calls,
    const std::string& tool_call_id)
    : role(role),
      type(type),
      content(std::move(content)),
      topic(topic),
      tool_calls(std::move(tool_calls)),
      tool_call_id(tool_call_id) {}

ConversationAPIClient::ConversationEvent::ConversationEvent() = default;

ConversationAPIClient::ConversationEvent::~ConversationEvent() = default;

ConversationAPIClient::ConversationEvent::ConversationEvent(
    ConversationAPIClient::ConversationEvent&& other) = default;

ConversationAPIClient::ConversationEvent& ConversationEvent::operator=(
    ConversationAPIClient::ConversationEvent&& other) = default;

ConversationAPIClient::ConversationAPIClient(
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

ConversationAPIClient::~ConversationAPIClient() = default;

void ConversationAPIClient::ClearAllQueries() {
  api_request_helper_->CancelAll();
}

void ConversationAPIClient::PerformRequest(
    std::vector<ConversationEvent> conversation,
    const std::string& selected_language,
    std::optional<base::Value::List> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    const std::optional<std::string>& model_name) {
  // Get credentials and then perform request
  auto callback = base::BindOnce(
      &ConversationAPIClient::PerformRequestWithCredentials,
      weak_ptr_factory_.GetWeakPtr(), std::move(conversation),
      selected_language, std::move(oai_tool_definitions), preferred_tool_name,
      model_name, std::move(data_received_callback),
      std::move(completed_callback));
  credential_manager_->FetchPremiumCredential(std::move(callback));
}

std::string ConversationAPIClient::CreateJSONRequestBody(
    std::vector<ConversationEvent> conversation,
    const std::string& selected_language,
    std::optional<base::Value::List> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    const std::optional<std::string>& model_name,
    const bool is_sse_enabled) {
  base::Value::Dict dict;

  dict.Set("events", ConversationEventsToList(std::move(conversation)));
  dict.Set("model", model_name ? *model_name : model_name_);
  dict.Set("selected_language", selected_language);
  dict.Set("system_language",
           base::StrCat({brave_l10n::GetDefaultISOLanguageCodeString(), "_",
                         brave_l10n::GetDefaultISOCountryCodeString()}));
  dict.Set("stream", is_sse_enabled);
#if !BUILDFLAG(IS_IOS)
  dict.Set("use_citations", true);
#endif

  if (oai_tool_definitions.has_value() && !oai_tool_definitions->empty()) {
    dict.Set("tools", std::move(oai_tool_definitions.value()));
  }

  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

void ConversationAPIClient::PerformRequestWithCredentials(
    std::vector<ConversationEvent> conversation,
    const std::string& selected_language,
    std::optional<base::Value::List> oai_tool_definitions,
    const std::optional<std::string>& preferred_tool_name,
    const std::optional<std::string>& model_name,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback,
    std::optional<CredentialCacheEntry> credential) {
  if (conversation.empty()) {
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
  const std::string request_body =
      CreateJSONRequestBody(std::move(conversation), selected_language,
                            std::move(oai_tool_definitions),
                            preferred_tool_name, model_name, is_sse_enabled);

  base::flat_map<std::string, std::string> headers;
  const auto digest_header = brave_service_keys::GetDigestHeader(request_body);
  headers.emplace(digest_header.first, digest_header.second);
  auto result = brave_service_keys::GetAuthorizationHeader(
      BUILDFLAG(SERVICE_KEY_AICHAT), headers, api_url,
      net::HttpRequestHeaders::kPostMethod, {"digest"});
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
    DVLOG(2) << "Making streaming AI Chat Conversation API Request";
    auto on_received = base::BindRepeating(
        &ConversationAPIClient::OnQueryDataReceived,
        weak_ptr_factory_.GetWeakPtr(), std::move(data_received_callback));
    auto on_complete =
        base::BindOnce(&ConversationAPIClient::OnQueryCompleted,
                       weak_ptr_factory_.GetWeakPtr(), std::move(credential),
                       std::move(completed_callback));

    api_request_helper_->RequestSSE(net::HttpRequestHeaders::kPostMethod,
                                    api_url, request_body, "application/json",
                                    std::move(on_received),
                                    std::move(on_complete), headers, {});
  } else {
    DVLOG(2) << "Making non-streaming AI Chat Conversation API Request";
    auto on_complete =
        base::BindOnce(&ConversationAPIClient::OnQueryCompleted,
                       weak_ptr_factory_.GetWeakPtr(), std::move(credential),
                       std::move(completed_callback));

    api_request_helper_->Request(net::HttpRequestHeaders::kPostMethod, api_url,
                                 request_body, "application/json",
                                 std::move(on_complete), headers, {});
  }
}

void ConversationAPIClient::OnQueryCompleted(
    std::optional<CredentialCacheEntry> credential,
    GenerationCompletedCallback callback,
    api_request_helper::APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  // Handle successful request
  if (success) {
    std::string completion = "";
    std::optional<std::string> model_key = std::nullopt;
    mojom::ConversationEntryEventPtr completion_event = nullptr;
    // We're checking for a value body in case for non-streaming API results.
    // TODO(petemill): server should provide parseable history events even for
    // non-streaming requests?
    if (result.value_body().is_dict()) {
      const std::string* value =
          result.value_body().GetDict().FindString("completion");
      if (value) {
        // Trimming necessary for Llama 2 which prepends responses with a " ".
        completion = base::TrimWhitespaceASCII(*value, base::TRIM_ALL);
      }

      const std::string* model_value =
          result.value_body().GetDict().FindString("model");
      if (model_value) {
        model_key = model_service_->GetLeoModelKeyByName(*model_value);
      }
    }

    completion_event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(completion));
    GenerationResultData data(std::move(completion_event),
                              std::move(model_key));
    std::move(callback).Run(base::ok(std::move(data)));
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

void ConversationAPIClient::OnQueryDataReceived(
    GenerationDataCallback callback,
    base::expected<base::Value, std::string> result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  auto& result_params = result->GetDict();

  if (auto result_data = ParseResponseEvent(result_params, model_service_)) {
    callback.Run(std::move(*result_data));
  }

  // Tool calls - they may happen individually or combined with a response event
  if (const base::Value::List* tool_calls =
          result_params.FindList("tool_calls")) {
    // Provide any valid tool use events to the callback
    for (auto& tool_use_event : ToolUseEventFromToolCallsResponse(tool_calls)) {
      auto tool_event = mojom::ConversationEntryEvent::NewToolUseEvent(
          std::move(tool_use_event));
      callback.Run(GenerationResultData(std::move(tool_event), std::nullopt));
    }
  }
}

// static
std::optional<ConversationAPIClient::GenerationResultData>
ConversationAPIClient::ParseResponseEvent(base::Value::Dict& response_event,
                                          ModelService* model_service) {
  mojom::ConversationEntryEventPtr event;
  const std::string* model = response_event.FindString("model");
  if (!model) {
    return std::nullopt;
  }

  const std::string* type = response_event.FindString("type");
  if (!type) {
    return std::nullopt;
  }

  // Vary response parsing based on type
  if (*type == "completion") {
    const std::string* completion = response_event.FindString("completion");
    if (!completion || completion->empty()) {
      return std::nullopt;
    }
    event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(*completion));
  } else if (*type == "isSearching") {
    event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
        mojom::SearchStatusEvent::New());
  } else if (*type == "searchQueries") {
    const base::Value::List* queries = response_event.FindList("queries");
    if (!queries) {
      return std::nullopt;
    }
    auto search_queries_event = mojom::SearchQueriesEvent::New();
    for (auto& item : *queries) {
      if (item.is_string()) {
        search_queries_event->search_queries.push_back(item.GetString());
      }
    }
    event = mojom::ConversationEntryEvent::NewSearchQueriesEvent(
        std::move(search_queries_event));
  } else if (*type == "webSources") {
    const base::Value::List* sources = response_event.FindList("sources");
    if (!sources) {
      return std::nullopt;
    }
    auto web_sources_event = mojom::WebSourcesEvent::New();
    for (auto& item : *sources) {
      if (!item.is_dict()) {
        continue;
      }
      const base::Value::Dict& source = item.GetDict();
      const std::string* title = source.FindString("title");
      const std::string* url = source.FindString("url");
      const std::string* favicon_url = source.FindString("favicon");
      if (!title || !url) {
        DVLOG(2) << "Missing required fields in web source event: "
                 << item.DebugString();
        continue;
      }
      GURL item_url(*url);
      GURL item_favicon_url =
          favicon_url
              ? GURL(*favicon_url)
              : GURL("chrome-untrusted://resources/brave-icons/globe.svg");

      if (!item_url.is_valid() || !item_favicon_url.is_valid()) {
        DVLOG(2) << "Invalid URL in webSource event: " << item.DebugString();
        continue;
      }
      // Validate favicon is private source
      if (favicon_url && (!item_favicon_url.SchemeIs(url::kHttpsScheme) ||
                          base::CompareCaseInsensitiveASCII(
                              item_favicon_url.host_piece(),
                              kAllowedWebSourceFaviconHost) != 0)) {
        DVLOG(2) << "webSource event contained disallowed host or scheme: "
                 << item.DebugString();
        continue;
      }
      web_sources_event->sources.push_back(
          mojom::WebSource::New(*title, item_url, item_favicon_url));
    }
    if (web_sources_event->sources.empty()) {
      return std::nullopt;
    }
    event = mojom::ConversationEntryEvent::NewSourcesEvent(
        std::move(web_sources_event));
  } else if (*type == "conversationTitle") {
    const std::string* title = response_event.FindString("title");
    if (!title) {
      return std::nullopt;
    }
    event = mojom::ConversationEntryEvent::NewConversationTitleEvent(
        mojom::ConversationTitleEvent::New(*title));
  } else if (*type == "selectedLanguage") {
    const std::string* selected_language =
        response_event.FindString("language");
    if (!selected_language) {
      return std::nullopt;
    }
    event = mojom::ConversationEntryEvent::NewSelectedLanguageEvent(
        mojom::SelectedLanguageEvent::New(*selected_language));
  } else if (*type == "contentReceipt") {
    std::optional<int> total_tokens_opt =
        response_event.FindInt("total_tokens");
    uint64_t total_tokens =
        total_tokens_opt.has_value() && total_tokens_opt.value() >= 0
            ? static_cast<uint64_t>(total_tokens_opt.value())
            : 0;
    std::optional<int> trimmed_tokens_opt =
        response_event.FindInt("trimmed_tokens");
    uint64_t trimmed_tokens =
        trimmed_tokens_opt.has_value() && trimmed_tokens_opt.value() >= 0
            ? static_cast<uint64_t>(trimmed_tokens_opt.value())
            : 0;
    event = mojom::ConversationEntryEvent::NewContentReceiptEvent(
        mojom::ContentReceiptEvent::New(total_tokens, trimmed_tokens));
  } else {
    // Server will provide different types of events. From time to time, new
    // types of events will be introduced and we should ignore unknown ones.
    return std::nullopt;
  }

  return GenerationResultData(std::move(event),
                              model_service->GetLeoModelKeyByName(*model));
}

}  // namespace ai_chat

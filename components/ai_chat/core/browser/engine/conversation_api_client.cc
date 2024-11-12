// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/types/expected.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/l10n/common/locale_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

using ConversationEvent = ConversationAPIClient::ConversationEvent;
using ConversationEventType = ConversationAPIClient::ConversationEventType;

constexpr char kRemotePath[] = "v1/conversation";
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

mojom::ConversationEntryEventPtr ParseResponseEvent(
    base::Value::Dict& response_event) {
  const std::string* type = response_event.FindString("type");
  if (!type) {
    return nullptr;
  }
  // Vary response parsing based on type
  if (*type == "completion") {
    const std::string* completion = response_event.FindString("completion");
    if (!completion) {
      return nullptr;
    }
    return mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(*completion));
  } else if (*type == "isSearching") {
    return mojom::ConversationEntryEvent::NewSearchStatusEvent(
        mojom::SearchStatusEvent::New());
  } else if (*type == "searchQueries") {
    const base::Value::List* queries = response_event.FindList("queries");
    if (!queries) {
      return nullptr;
    }
    auto event = mojom::SearchQueriesEvent::New();
    for (auto& item : *queries) {
      if (item.is_string()) {
        event->search_queries.push_back(item.GetString());
      }
    }
    return mojom::ConversationEntryEvent::NewSearchQueriesEvent(
        std::move(event));
  } else if (*type == "conversationTitle") {
    const std::string* title = response_event.FindString("title");
    if (!title) {
      return nullptr;
    }
    return mojom::ConversationEntryEvent::NewConversationTitleEvent(
        mojom::ConversationTitleEvent::New(*title));
  } else if (*type == "selectedLanguage") {
    const std::string* selected_language =
        response_event.FindString("language");
    if (!selected_language) {
      return nullptr;
    }
    return mojom::ConversationEntryEvent::NewSelectedLanguageEvent(
        mojom::SelectedLanguageEvent::New(*selected_language));
  }
  // Server will provide different types of events. From time to time, new
  // types of events will be introduced and we should ignore unknown ones.
  return nullptr;
}

base::Value::List ConversationEventsToList(
    const std::vector<ConversationEvent>& conversation) {
  static const base::NoDestructor<std::map<mojom::CharacterType, std::string>>
      kRoleMap({{mojom::CharacterType::HUMAN, "user"},
                {mojom::CharacterType::ASSISTANT, "assistant"}});

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
           {ConversationEventType::SuggestedActions, "suggestedActions"}});

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

    event_dict.Set("content", event.content);
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

ConversationAPIClient::ConversationAPIClient(
    const std::string& model_name,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager)
    : model_name_(model_name), credential_manager_(credential_manager) {
  CHECK(!model_name_.empty());
  api_request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      GetNetworkTrafficAnnotationTag(), url_loader_factory);
}

ConversationAPIClient::~ConversationAPIClient() = default;

void ConversationAPIClient::ClearAllQueries() {
  api_request_helper_->CancelAll();
}

void ConversationAPIClient::PerformRequest(
    const std::vector<ConversationEvent>& conversation,
    const std::string& selected_language,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback) {
  // Get credentials and then perform request
  auto callback =
      base::BindOnce(&ConversationAPIClient::PerformRequestWithCredentials,
                     weak_ptr_factory_.GetWeakPtr(), std::move(conversation),
                     selected_language, std::move(data_received_callback),
                     std::move(completed_callback));
  credential_manager_->FetchPremiumCredential(std::move(callback));
}

std::string ConversationAPIClient::CreateJSONRequestBody(
    const std::vector<ConversationEvent>& conversation,
    const std::string& selected_language,
    const bool is_sse_enabled) {
  base::Value::Dict dict;

  dict.Set("events", ConversationEventsToList(conversation));
  dict.Set("model", model_name_);
  dict.Set("selected_language", selected_language);
  dict.Set("system_language",
           base::StrCat({brave_l10n::GetDefaultISOLanguageCodeString(), "_",
                         brave_l10n::GetDefaultISOCountryCodeString()}));
  base::StrCat({brave_l10n::GetDefaultISOLanguageCodeString(), "_",
                brave_l10n::GetDefaultISOCountryCodeString()});
  dict.Set("stream", is_sse_enabled);

  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

void ConversationAPIClient::PerformRequestWithCredentials(
    const std::vector<ConversationEvent>& conversation,
    const std::string selected_language,
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
  const std::string request_body = CreateJSONRequestBody(
      std::move(conversation), selected_language, is_sse_enabled);

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
    APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  // Handle successful request
  if (success) {
    std::string completion = "";
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
    }

    std::move(callback).Run(base::ok(std::move(completion)));
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
  auto event = ParseResponseEvent(result->GetDict());
  if (event) {
    callback.Run(std::move(event));
  }
}

}  // namespace ai_chat

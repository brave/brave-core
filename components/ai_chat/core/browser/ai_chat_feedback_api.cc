/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_feedback_api.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/l10n/common/locale_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {
namespace {

constexpr char kFeedbackHostnamePart[] = "feedback";
constexpr char kRatingPath[] = "1/ai/feedback/rating";
constexpr char kFeedbackFormPath[] = "1/ai/feedback/form";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat Feedback"
        description:
          "This is used to communicate with a service to record a rating"
          "of an AI Chat message, and anonymous details about that message."
        trigger:
          "Triggered by user choosing a rating for a received AI message."
        data:
          "Positive or negative rating along with an extra details the user
          provides, and the associated chat conversation messages."
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
  auto domain = brave_domains::GetServicesDomain(kFeedbackHostnamePart);

  static base::NoDestructor<GURL> url{
      base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator, domain})};
  return *url;
}

}  // namespace

AIChatFeedbackAPI::AIChatFeedbackAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const std::string& channel_name)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      channel_name_(channel_name) {}

AIChatFeedbackAPI::~AIChatFeedbackAPI() = default;

void AIChatFeedbackAPI::SendRating(
    bool is_liked,
    bool is_premium,
    const base::span<const mojom::ConversationTurnPtr>& history,
    const std::string& model_name,
    api_request_helper::APIRequestHelper::ResultCallback on_complete_callback) {
  base::Value::Dict payload;

  base::Value::List chat;
  int id = 0;
  for (auto& turn : history) {
    base::Value::Dict turn_dict;
    turn_dict.Set("id", id);
    turn_dict.Set("type", turn->character_type == mojom::CharacterType::HUMAN
                              ? "human"
                              : "assistant");
    turn_dict.Set("content", turn->text);
    ++id;

    chat.Append(std::move(turn_dict));
  }

  payload.Set("chat", std::move(chat));
  payload.Set("ymd", brave_stats::GetDateAsYMD(base::Time::Now()));
  payload.Set("model", model_name);
  payload.Set("locale",
              base::StrCat({brave_l10n::GetDefaultISOLanguageCodeString(), "_",
                            brave_l10n::GetDefaultISOCountryCodeString()}));
  payload.Set("rating", static_cast<int>(is_liked));
  payload.Set("channel", channel_name_);
  payload.Set("platform", brave_stats::GetPlatformIdentifier());
  payload.Set("is_premium", is_premium);

  base::flat_map<std::string, std::string> headers;
  headers.emplace("Accept", "application/json");

  GURL api_url = GetEndpointBaseUrl().Resolve(kRatingPath);

  api_request_helper_.Request("POST", api_url, CreateJSONRequestBody(payload),
                              "application/json",
                              std::move(on_complete_callback), headers);
}

void AIChatFeedbackAPI::SendFeedback(
    const std::string& category,
    const std::string& feedback,
    const std::string& rating_id,
    const std::optional<std::string>& hostname,
    api_request_helper::APIRequestHelper::ResultCallback on_complete_callback) {
  base::Value::Dict dict;

  dict.Set("ymd", brave_stats::GetDateAsYMD(base::Time::Now()));
  dict.Set("category", category);
  dict.Set("feedback", feedback);
  dict.Set("rating_id", rating_id);
  dict.Set("locale",
           base::StrCat({brave_l10n::GetDefaultISOLanguageCodeString(), "_",
                         brave_l10n::GetDefaultISOCountryCodeString()}));

  if (hostname.has_value()) {
    dict.Set("domain", hostname.value());
  }

  GURL api_url = GetEndpointBaseUrl().Resolve(kFeedbackFormPath);

  api_request_helper_.Request("POST", api_url, CreateJSONRequestBody(dict),
                              "application/json",
                              std::move(on_complete_callback));
}

}  // namespace ai_chat

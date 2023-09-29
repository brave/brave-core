/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_feedback_api.h"

#include <base/containers/flat_map.h>

#include <utility>

#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/brave_domains/buildflags.h"
#include "brave/components/l10n/common/locale_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace ai_chat {
namespace {

constexpr char kRatingPath[] = "1/ai/feedback/rating";
constexpr char kFeedbackFormPath[] = "1/ai/feedback/form";

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

base::Value::Dict ToRatingPayloadDict(const RatingPayload& rating_payload) {
  base::Value::Dict dict;
  dict.Set("id", rating_payload.id);
  dict.Set("message", rating_payload.message);

  return dict;
}

const GURL GetEndpointBaseUrl() {
  auto* base_hostname = BUILDFLAG(BRAVE_SERVICES_PRODUCTION_DOMAIN);

  static base::NoDestructor<GURL> url{
      base::StrCat({url::kHttpsScheme, "://feedback.", base_hostname})};
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
    RatingPayload rating_payload,
    const std::string& model_name,
    api_request_helper::APIRequestHelper::ResultCallback on_complete_callback) {
  base::Value::Dict dict;
  base::Value::List chat;

  chat.Append(ToRatingPayloadDict(rating_payload));

  std::string locale =
      base::StrCat({brave_l10n::GetDefaultISOLanguageCodeString(), "_",
                    brave_l10n::GetDefaultISOCountryCodeString()});

  dict.Set("ymd", brave_stats::GetDateAsYMD(base::Time::Now()));
  dict.Set("chat", std::move(chat));
  dict.Set("model", model_name);
  dict.Set("locale", locale);
  dict.Set("rating", static_cast<int>(is_liked));
  dict.Set("channel", channel_name_);
  dict.Set("platform", brave_stats::GetPlatformIdentifier());

  base::flat_map<std::string, std::string> headers;
  headers.emplace("Accept", "application/json");

  GURL api_url = GetEndpointBaseUrl().Resolve(kRatingPath);

  api_request_helper_.Request("POST", api_url, CreateJSONRequestBody(dict),
                              "application/json",
                              std::move(on_complete_callback), headers);
}

void AIChatFeedbackAPI::SendFeedback(
    const std::string& category,
    const std::string& feedback,
    const std::string& rating_id,
    api_request_helper::APIRequestHelper::ResultCallback on_complete_callback) {
  base::Value::Dict dict;

  dict.Set("ymd", brave_stats::GetDateAsYMD(base::Time::Now()));
  dict.Set("category", category);
  dict.Set("feedback", feedback);
  dict.Set("rating_id", rating_id);

  GURL api_url = GetEndpointBaseUrl().Resolve(kFeedbackFormPath);

  api_request_helper_.Request("POST", api_url, CreateJSONRequestBody(dict),
                              "application/json",
                              std::move(on_complete_callback));
}

}  // namespace ai_chat

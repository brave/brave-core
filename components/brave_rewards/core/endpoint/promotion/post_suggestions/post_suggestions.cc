/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/post_suggestions/post_suggestions.h"

#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PostSuggestions::PostSuggestions(RewardsEngineImpl& engine) : engine_(engine) {}

PostSuggestions::~PostSuggestions() = default;

std::string PostSuggestions::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .rewards_grant_url()
      .Resolve("/v1/suggestions")
      .spec();
}

std::string PostSuggestions::GeneratePayload(
    const credential::CredentialsRedeem& redeem) {
  base::Value::Dict data;
  data.Set("type", credential::ConvertRewardTypeToString(redeem.type));
  if (!redeem.order_id.empty()) {
    data.Set("orderId", redeem.order_id);
  }
  data.Set("channel", redeem.publisher_key);

  const bool is_sku = redeem.processor == mojom::ContributionProcessor::UPHOLD;

  std::string data_json;
  base::JSONWriter::Write(data, &data_json);
  std::string data_encoded;
  base::Base64Encode(data_json, &data_encoded);

  base::Value::List credentials =
      credential::GenerateCredentials(redeem.token_list, data_encoded);

  const std::string data_key = is_sku ? "vote" : "suggestion";
  base::Value::Dict payload;
  payload.Set(data_key, data_encoded);
  payload.Set("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

mojom::Result PostSuggestions::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return mojom::Result::BAD_REGISTRATION_RESPONSE;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PostSuggestions::Request(const credential::CredentialsRedeem& redeem,
                              PostSuggestionsCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(redeem);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&PostSuggestions::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PostSuggestions::OnRequest(PostSuggestionsCallback callback,
                                mojom::UrlResponsePtr response) {
  DCHECK(response);
  callback(CheckStatusCode(response->status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal

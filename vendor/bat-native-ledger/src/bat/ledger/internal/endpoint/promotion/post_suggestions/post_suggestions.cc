/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_suggestions/post_suggestions.h"

#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

PostSuggestions::PostSuggestions(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostSuggestions::~PostSuggestions() = default;

std::string PostSuggestions::GetUrl() {
  return GetServerUrl("/v1/suggestions");
}

std::string PostSuggestions::GeneratePayload(
    const credential::CredentialsRedeem& redeem) {
  base::Value::Dict data;
  data.Set("type", credential::ConvertRewardTypeToString(redeem.type));
  if (!redeem.order_id.empty()) {
    data.Set("orderId", redeem.order_id);
  }
  data.Set("channel", redeem.publisher_key);

  const bool is_sku = redeem.processor == type::ContributionProcessor::UPHOLD;

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

type::Result PostSuggestions::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return type::Result::BAD_REGISTRATION_RESPONSE;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

void PostSuggestions::Request(
    const credential::CredentialsRedeem& redeem,
    PostSuggestionsCallback callback) {
  auto url_callback = std::bind(&PostSuggestions::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(redeem);
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostSuggestions::OnRequest(
    const type::UrlResponse& response,
    PostSuggestionsCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

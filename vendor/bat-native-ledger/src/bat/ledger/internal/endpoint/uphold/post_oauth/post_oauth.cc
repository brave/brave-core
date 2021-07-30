/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/post_oauth/post_oauth.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace uphold {

PostOauth::PostOauth(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostOauth::~PostOauth() = default;

std::string PostOauth::GetUrl() {
  return GetServerUrl("/oauth2/token");
}

std::string PostOauth::GeneratePayload(const std::string& code) {
  return base::StringPrintf(
      "code=%s&grant_type=authorization_code",
      code.c_str());
}

type::Result PostOauth::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Unauthorized access");
    return type::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result PostOauth::ParseBody(
    const std::string& body,
    std::string* token) {
  DCHECK(token);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  const auto* access_token = dictionary->FindStringKey("access_token");
  if (!access_token) {
    BLOG(0, "Missing access token");
    return type::Result::LEDGER_ERROR;
  }

  *token = *access_token;

  return type::Result::LEDGER_OK;
}

void PostOauth::Request(
    const std::string& code,
    PostOauthCallback callback) {
  auto url_callback = std::bind(&PostOauth::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(code);
  request->headers = RequestAuthorization();
  request->content_type = "application/x-www-form-urlencoded";
  request->method = type::UrlMethod::POST;
  request->skip_log = true;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostOauth::OnRequest(
    const type::UrlResponse& response,
    PostOauthCallback callback) {
  ledger::LogUrlResponse(__func__, response, true);

  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, "");
    return;
  }

  std::string token;
  result = ParseBody(response.body, &token);
  callback(result, token);
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/bitflyer/post_oauth/post_oauth_bitflyer.h"

#include <utility>

#include "base/base64url.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "crypto/sha2.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace bitflyer {

PostOauth::PostOauth(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

PostOauth::~PostOauth() = default;

std::string PostOauth::GetUrl() {
  return GetServerUrl("/api/link/v1/token");
}

std::string PostOauth::GeneratePayload(const std::string& external_account_id,
                                       const std::string& code) {
  const std::string client_id = ledger::bitflyer::GetClientId();
  const std::string client_secret = GetClientSecret();
  const std::string request_id = base::GenerateGUID();

  base::DictionaryValue dict;
  dict.SetStringKey("grant_type", "code");
  dict.SetStringKey("code", code);
  dict.SetStringKey("client_id", client_id);
  dict.SetStringKey("client_secret", client_secret);
  dict.SetIntKey("expires_in", 259002);
  dict.SetStringKey("external_account_id", external_account_id);
  dict.SetStringKey("request_id", request_id);
  dict.SetStringKey("redirect_uri", "rewards://bitflyer/authorization");
  dict.SetBoolKey("request_deposit_id", true);

  // Send PKCE code verifier and challenge when running in production
  if (ledger::_environment == ledger::type::Environment::PRODUCTION) {
    const std::string code_verifier =
        ledger::bitflyer::GenerateRandomString(ledger::is_testing);
    const std::string hashed_code_verifier =
        crypto::SHA256HashString(code_verifier);
    std::string code_challenge;
    base::Base64UrlEncode(hashed_code_verifier,
                          base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                          &code_challenge);
    dict.SetStringKey("code_verifier", code_verifier);
    dict.SetStringKey("code_challenge_method", "S256");
    dict.SetStringKey("code_challenge", code_challenge);
  }

  std::string payload;
  base::JSONWriter::Write(dict, &payload);
  return payload;
}

type::Result PostOauth::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    return type::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result PostOauth::ParseBody(const std::string& body,
                                  std::string* token,
                                  std::string* address,
                                  std::string* linking_info) {
  DCHECK(token);
  DCHECK(address);
  DCHECK(linking_info);

  base::Optional<base::Value> value = base::JSONReader::Read(body);
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

  const auto* deposit_id = dictionary->FindStringKey("deposit_id");
  if (!deposit_id) {
    BLOG(0, "Missing deposit id");
    return type::Result::LEDGER_ERROR;
  }

  const auto* linking_information = dictionary->FindStringKey("linking_info");
  if (!linking_information) {
    BLOG(0, "Missing linking info");
    return type::Result::LEDGER_ERROR;
  }

  *token = *access_token;
  *address = *deposit_id;
  *linking_info = *linking_information;

  return type::Result::LEDGER_OK;
}

void PostOauth::Request(const std::string& external_account_id,
                        const std::string& code,
                        PostOauthCallback callback) {
  auto url_callback = std::bind(&PostOauth::OnRequest, this, _1, callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(external_account_id, code);
  request->headers = RequestAuthorization();
  request->content_type = "application/json";
  request->method = type::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostOauth::OnRequest(const type::UrlResponse& response,
                          PostOauthCallback callback) {
  ledger::LogUrlResponse(__func__, response, true);

  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, "", "", "");
    return;
  }

  std::string token;
  std::string address;
  std::string linking_info;
  result = ParseBody(response.body, &token, &address, &linking_info);
  callback(result, token, address, linking_info);
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger

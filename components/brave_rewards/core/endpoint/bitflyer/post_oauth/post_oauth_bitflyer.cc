/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/bitflyer/post_oauth/post_oauth_bitflyer.h"

#include <utility>

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::bitflyer {

PostOauth::PostOauth(LedgerImpl& ledger) : ledger_(ledger) {}

PostOauth::~PostOauth() = default;

std::string PostOauth::GetUrl() {
  return GetServerUrl("/api/link/v1/token");
}

std::string PostOauth::GeneratePayload(const std::string& external_account_id,
                                       const std::string& code,
                                       const std::string& code_verifier) {
  const std::string client_id = internal::bitflyer::GetClientId();
  const std::string client_secret = internal::bitflyer::GetClientSecret();
  const std::string request_id = base::GenerateGUID();

  base::Value::Dict dict;
  dict.Set("grant_type", "code");
  dict.Set("code", code);
  dict.Set("code_verifier", code_verifier);
  dict.Set("client_id", client_id);
  dict.Set("client_secret", client_secret);
  dict.Set("expires_in", 259002);
  dict.Set("external_account_id", external_account_id);
  dict.Set("request_id", request_id);
  dict.Set("redirect_uri", "rewards://bitflyer/authorization");
  dict.Set("request_deposit_id", true);

  std::string payload;
  base::JSONWriter::Write(dict, &payload);
  return payload;
}

mojom::Result PostOauth::CheckStatusCode(int status_code) {
  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

mojom::Result PostOauth::ParseBody(const std::string& body,
                                   std::string* token,
                                   std::string* address,
                                   std::string* linking_info) {
  DCHECK(token);
  DCHECK(address);
  DCHECK(linking_info);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* access_token = dict.FindString("access_token");
  if (!access_token) {
    BLOG(0, "Missing access token");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* deposit_id = dict.FindString("deposit_id");
  if (!deposit_id) {
    BLOG(0, "Missing deposit id");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* linking_information = dict.FindString("linking_info");
  if (!linking_information) {
    BLOG(0, "Missing linking info");
    return mojom::Result::LEDGER_ERROR;
  }

  *token = *access_token;
  *address = *deposit_id;
  *linking_info = *linking_information;

  return mojom::Result::LEDGER_OK;
}

void PostOauth::Request(const std::string& external_account_id,
                        const std::string& code,
                        const std::string& code_verifier,
                        PostOauthCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(external_account_id, code, code_verifier);
  request->headers = RequestAuthorization();
  request->content_type = "application/json";
  request->method = mojom::UrlMethod::POST;
  request->skip_log = true;

  ledger_->LoadURL(std::move(request),
                   base::BindOnce(&PostOauth::OnRequest, base::Unretained(this),
                                  std::move(callback)));
}

void PostOauth::OnRequest(PostOauthCallback callback,
                          mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response, true);

  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::LEDGER_OK) {
    return std::move(callback).Run(result, "", "", "");
  }

  std::string token;
  std::string address;
  std::string linking_info;
  result = ParseBody(response->body, &token, &address, &linking_info);
  std::move(callback).Run(result, std::move(token), std::move(address),
                          std::move(linking_info));
}

}  // namespace brave_rewards::internal::endpoint::bitflyer

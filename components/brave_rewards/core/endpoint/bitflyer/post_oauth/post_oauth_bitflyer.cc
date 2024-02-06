/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/bitflyer/post_oauth/post_oauth_bitflyer.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/uuid.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::bitflyer {

PostOauth::PostOauth(RewardsEngineImpl& engine) : engine_(engine) {}

PostOauth::~PostOauth() = default;

std::string PostOauth::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .bitflyer_url()
      .Resolve("/api/link/v1/token")
      .spec();
}

std::string PostOauth::GeneratePayload(const std::string& external_account_id,
                                       const std::string& code,
                                       const std::string& code_verifier) {
  auto& config = engine_->Get<EnvironmentConfig>();

  const std::string request_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();

  base::Value::Dict dict;
  dict.Set("grant_type", "code");
  dict.Set("code", code);
  dict.Set("code_verifier", code_verifier);
  dict.Set("client_id", config.bitflyer_client_id());
  dict.Set("client_secret", config.bitflyer_client_secret());
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
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result PostOauth::ParseBody(const std::string& body,
                                   std::string* token,
                                   std::string* address,
                                   std::string* linking_info) {
  DCHECK(token);
  DCHECK(address);
  DCHECK(linking_info);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine_->LogError(FROM_HERE) << "Invalid JSON";
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* access_token = dict.FindString("access_token");
  if (!access_token) {
    engine_->LogError(FROM_HERE) << "Missing access token";
    return mojom::Result::FAILED;
  }

  const auto* deposit_id = dict.FindString("deposit_id");
  if (!deposit_id) {
    engine_->LogError(FROM_HERE) << "Missing deposit id";
    return mojom::Result::FAILED;
  }

  const auto* linking_information = dict.FindString("linking_info");
  if (!linking_information) {
    engine_->LogError(FROM_HERE) << "Missing linking info";
    return mojom::Result::FAILED;
  }

  *token = *access_token;
  *address = *deposit_id;
  *linking_info = *linking_information;

  return mojom::Result::OK;
}

void PostOauth::Request(const std::string& external_account_id,
                        const std::string& code,
                        const std::string& code_verifier,
                        PostOauthCallback callback) {
  auto get_auth_user = [&]() {
    auto& config = engine_->Get<EnvironmentConfig>();
    std::string user;
    base::Base64Encode(base::StrCat({config.bitflyer_client_id(), ":",
                                     config.bitflyer_client_secret()}),
                       &user);
    return user;
  };

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(external_account_id, code, code_verifier);
  request->headers = {"Authorization: Basic " + get_auth_user()};
  request->content_type = "application/json";
  request->method = mojom::UrlMethod::POST;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kNone,
      base::BindOnce(&PostOauth::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PostOauth::OnRequest(PostOauthCallback callback,
                          mojom::UrlResponsePtr response) {
  DCHECK(response);

  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::OK) {
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

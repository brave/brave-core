/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/post_oauth/post_oauth_gemini.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/uuid.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal::endpoint::gemini {

PostOauth::PostOauth(RewardsEngineImpl& engine) : engine_(engine) {}

PostOauth::~PostOauth() = default;

std::string PostOauth::GetUrl() {
  return GetOauthServerUrl("/auth/token");
}

std::string PostOauth::GeneratePayload(const std::string& external_account_id,
                                       const std::string& code) {
  const std::string client_id = internal::gemini::GetClientId();
  const std::string client_secret = internal::gemini::GetClientSecret();
  const std::string request_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();

  base::Value::Dict dict;
  dict.Set("client_id", client_id);
  dict.Set("client_secret", client_secret);
  dict.Set("code", code);
  dict.Set("redirect_uri", "rewards://gemini/authorization");
  dict.Set("grant_type", "authorization_code");

  std::string payload;
  base::JSONWriter::Write(dict, &payload);
  return payload;
}

mojom::Result PostOauth::ParseBody(const std::string& body,
                                   std::string* token) {
  DCHECK(token);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* access_token = dict.FindString("access_token");
  if (!access_token) {
    BLOG(0, "Missing access token");
    return mojom::Result::FAILED;
  }

  *token = *access_token;
  return mojom::Result::OK;
}

void PostOauth::Request(const std::string& external_account_id,
                        const std::string& code,
                        PostOauthCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(external_account_id, code);
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
    return std::move(callback).Run(result, "");
  }

  std::string token;
  result = ParseBody(response->body, &token);
  std::move(callback).Run(result, std::move(token));
}

}  // namespace brave_rewards::internal::endpoint::gemini

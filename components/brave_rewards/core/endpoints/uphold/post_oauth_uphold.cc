/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/post_oauth_uphold.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostOAuthUphold::Error;
using Result = PostOAuthUphold::Result;

namespace {

Result ParseBody(const std::string& body) {
  auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  auto* access_token = value->GetDict().FindString("access_token");
  if (!access_token || access_token->empty()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  return std::move(*access_token);
}

}  // namespace

// static
Result PostOAuthUphold::ProcessResponse(const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(response.body);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

PostOAuthUphold::PostOAuthUphold(RewardsEngineImpl& engine,
                                 const std::string& code)
    : RequestBuilder(engine), code_(code) {}

PostOAuthUphold::~PostOAuthUphold() = default;

std::optional<std::string> PostOAuthUphold::Url() const {
  return engine_->Get<EnvironmentConfig>()
      .uphold_api_url()
      .Resolve("/oauth2/token")
      .spec();
}

std::optional<std::vector<std::string>> PostOAuthUphold::Headers(
    const std::string&) const {
  auto& config = engine_->Get<EnvironmentConfig>();

  std::string user;
  base::Base64Encode(base::StrCat({config.uphold_client_id(), ":",
                                   config.uphold_client_secret()}),
                     &user);

  return std::vector<std::string>{"Authorization: Basic " + user};
}

std::optional<std::string> PostOAuthUphold::Content() const {
  if (code_.empty()) {
    BLOG(0, "code_ is empty!");
    return std::nullopt;
  }

  return "code=" + code_ + "&grant_type=authorization_code";
}

std::string PostOAuthUphold::ContentType() const {
  return "application/x-www-form-urlencoded";
}

bool PostOAuthUphold::SkipLog() const {
  return true;
}

}  // namespace brave_rewards::internal::endpoints

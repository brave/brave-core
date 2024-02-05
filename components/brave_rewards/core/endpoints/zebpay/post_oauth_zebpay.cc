/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/zebpay/post_oauth_zebpay.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostOAuthZebPay::Error;
using Result = PostOAuthZebPay::Result;

namespace {

base::expected<std::pair<std::string, std::string>, Error>
GetAccessTokenAndLinkingInfo(const std::string& body) {
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

  auto* linking_info = value->GetDict().FindString("linking_info");
  if (!linking_info || linking_info->empty()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  return std::pair{std::move(*access_token), std::move(*linking_info)};
}

Result ParseBody(const std::string& body) {
  auto access_token_linking_info = GetAccessTokenAndLinkingInfo(body);
  if (!access_token_linking_info.has_value()) {
    return base::unexpected(access_token_linking_info.error());
  }

  auto [access_token, linking_info] =
      std::move(access_token_linking_info.value());

  std::vector<std::size_t> dots;
  for (auto pos = linking_info.find('.'); pos != std::string::npos;
       pos = linking_info.find('.', pos + 1)) {
    dots.push_back(pos);
  }

  if (dots.size() != 2) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  std::string payload;
  if (!base::Base64Decode(
          linking_info.substr(dots[0] + 1, dots[1] - dots[0] - 1), &payload,
          base::Base64DecodePolicy::kForgiving)) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  auto value = base::JSONReader::Read(payload);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  auto* deposit_id = value->GetDict().FindString("depositId");
  if (!deposit_id || deposit_id->empty()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  return std::tuple{std::move(access_token), std::move(linking_info),
                    std::move(*deposit_id)};
}

}  // namespace

// static
Result PostOAuthZebPay::ProcessResponse(const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(response.body);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

PostOAuthZebPay::PostOAuthZebPay(RewardsEngineImpl& engine,
                                 const std::string& code)
    : RequestBuilder(engine), code_(code) {}

PostOAuthZebPay::~PostOAuthZebPay() = default;

std::optional<std::string> PostOAuthZebPay::Url() const {
  return engine_->Get<EnvironmentConfig>()
      .zebpay_oauth_url()
      .Resolve("/connect/token")
      .spec();
}

std::optional<std::vector<std::string>> PostOAuthZebPay::Headers(
    const std::string&) const {
  auto& config = engine_->Get<EnvironmentConfig>();
  std::string user;
  base::Base64Encode(base::StrCat({config.zebpay_client_id(), ":",
                                   config.zebpay_client_secret()}),
                     &user);
  return std::vector<std::string>{"Authorization: Basic " + user};
}

std::optional<std::string> PostOAuthZebPay::Content() const {
  if (code_.empty()) {
    BLOG(0, "code_ is empty!");
    return std::nullopt;
  }

  return "grant_type=authorization_code"
         "&redirect_uri=rewards://zebpay/authorization"
         "&code=" +
         code_;
}

std::string PostOAuthZebPay::ContentType() const {
  return "application/x-www-form-urlencoded";
}

bool PostOAuthZebPay::SkipLog() const {
  return true;
}

}  // namespace brave_rewards::internal::endpoints

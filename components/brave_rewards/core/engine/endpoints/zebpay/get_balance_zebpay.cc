/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/endpoints/zebpay/get_balance_zebpay.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/engine/rewards_engine.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = GetBalanceZebPay::Error;
using Result = GetBalanceZebPay::Result;

namespace {

Result ParseBody(RewardsEngine& engine, const std::string& body) {
  const auto value = base::JSONReader::ReadDict(body);
  if (!value) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  const auto balance = value->FindDouble("BAT");
  if (!balance) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  return *balance;
}

}  // namespace

// static
Result GetBalanceZebPay::ProcessResponse(RewardsEngine& engine,
                                         const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(engine, response.body);
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      engine.LogError(FROM_HERE) << "Access token expired";
      return base::unexpected(Error::kAccessTokenExpired);
    default:
      engine.LogError(FROM_HERE)
          << "Unexpected status code! (HTTP " << response.status_code << ')';
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

GetBalanceZebPay::GetBalanceZebPay(RewardsEngine& engine, std::string&& token)
    : RequestBuilder(engine), token_(std::move(token)) {}

GetBalanceZebPay::~GetBalanceZebPay() = default;

std::optional<std::string> GetBalanceZebPay::Url() const {
  return engine_->Get<EnvironmentConfig>()
      .zebpay_api_url()
      .Resolve("/api/balance")
      .spec();
}

mojom::UrlMethod GetBalanceZebPay::Method() const {
  return mojom::UrlMethod::GET;
}

std::optional<std::vector<std::string>> GetBalanceZebPay::Headers(
    const std::string&) const {
  return std::vector<std::string>{"Authorization: Bearer " + token_};
}

}  // namespace brave_rewards::internal::endpoints

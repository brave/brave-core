/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/get_transaction_status_uphold.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = GetTransactionStatusUphold::Error;
using Result = GetTransactionStatusUphold::Result;

namespace {

Result ParseBody(RewardsEngineImpl& engine, const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  const auto* status = value->GetDict().FindString("status");
  if (!status || status->empty()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  if (*status == "processing") {
    return base::unexpected(Error::kTransactionPending);
  }

  if (*status != "completed") {
    return base::unexpected(Error::kUnexpectedTransactionStatus);
  }

  return {};
}

}  // namespace

// static
Result GetTransactionStatusUphold::ProcessResponse(
    RewardsEngineImpl& engine,
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

std::optional<std::string> GetTransactionStatusUphold::Url() const {
  auto url =
      URLHelpers::Resolve(engine_->Get<EnvironmentConfig>().uphold_api_url(),
                          {"/v0/me/transactions/", transaction_id_});
  return url.spec();
}

std::optional<std::vector<std::string>> GetTransactionStatusUphold::Headers(
    const std::string&) const {
  return std::vector<std::string>{"Authorization: Bearer " + token_};
}

}  // namespace brave_rewards::internal::endpoints

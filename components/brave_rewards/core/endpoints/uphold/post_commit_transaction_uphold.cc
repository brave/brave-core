/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/post_commit_transaction_uphold.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostCommitTransactionUphold::Error;
using Result = PostCommitTransactionUphold::Result;

namespace {

Result ParseBody(RewardsEngine& engine, const std::string& body) {
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
Result PostCommitTransactionUphold::ProcessResponse(
    RewardsEngine& engine,
    const mojom::UrlResponse& response) {
  if (URLLoader::IsSuccessCode(response.status_code)) {
    return ParseBody(engine, response.body);
  }
  switch (response.status_code) {
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      engine.LogError(FROM_HERE) << "Access token expired";
      return base::unexpected(Error::kAccessTokenExpired);
    case net::HTTP_NOT_FOUND:  // HTTP 404
      engine.LogError(FROM_HERE) << "Transaction not found";
      return base::unexpected(Error::kTransactionNotFound);
    default:
      engine.LogError(FROM_HERE)
          << "Unexpected status code! (HTTP " << response.status_code << ')';
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

std::optional<std::string> PostCommitTransactionUphold::Url() const {
  return engine_->Get<EnvironmentConfig>()
      .uphold_api_url()
      .Resolve(base::StrCat({"/v0/me/cards/", address_, "/transactions/",
                             transaction_->transaction_id, "/commit"}))
      .spec();
}

std::optional<std::vector<std::string>> PostCommitTransactionUphold::Headers(
    const std::string&) const {
  return std::vector<std::string>{"Authorization: Bearer " + token_};
}

}  // namespace brave_rewards::internal::endpoints

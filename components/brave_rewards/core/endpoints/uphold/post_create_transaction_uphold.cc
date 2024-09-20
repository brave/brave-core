/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/post_create_transaction_uphold.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostCreateTransactionUphold::Error;
using Result = PostCreateTransactionUphold::Result;

namespace {

Result ParseBody(RewardsEngine& engine, const std::string& body) {
  auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  auto* id = value->GetDict().FindString("id");
  if (!id || id->empty()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  return std::move(*id);
}

}  // namespace

// static
Result PostCreateTransactionUphold::ProcessResponse(
    RewardsEngine& engine,
    const mojom::UrlResponse& response) {
  if (URLLoader::IsSuccessCode(response.status_code)) {
    return ParseBody(engine, response.body);
  }
  switch (response.status_code) {
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      engine.LogError(FROM_HERE) << "Access token expired";
      return base::unexpected(Error::kAccessTokenExpired);
    default:
      engine.LogError(FROM_HERE)
          << "Unexpected status code! (HTTP " << response.status_code << ')';
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

std::optional<std::string> PostCreateTransactionUphold::Url() const {
  return engine_->Get<EnvironmentConfig>()
      .uphold_api_url()
      .Resolve(base::StrCat({"/v0/me/cards/", address_, "/transactions"}))
      .spec();
}

std::optional<std::vector<std::string>> PostCreateTransactionUphold::Headers(
    const std::string&) const {
  return std::vector<std::string>{"Authorization: Bearer " + token_};
}

std::optional<std::string> PostCreateTransactionUphold::Content() const {
  base::Value::Dict denomination;
  denomination.Set("amount", transaction_->amount);
  denomination.Set("currency", "BAT");

  base::Value::Dict payload;
  payload.Set("destination", transaction_->destination);
  payload.Set("denomination", std::move(denomination));

  auto& config = engine_->Get<EnvironmentConfig>();
  if (transaction_->destination == config.uphold_fee_address()) {
    payload.Set("message", kFeeMessage);
  }

  std::string json;
  if (!base::JSONWriter::Write(payload, &json)) {
    return std::nullopt;
  }

  return json;
}

}  // namespace brave_rewards::internal::endpoints

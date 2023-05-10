/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/bitflyer/post_commit_transaction/post_commit_transaction_bitflyer.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostCommitTransactionBitFlyer::Error;
using Result = PostCommitTransactionBitFlyer::Result;

namespace {

Result ParseBody(const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  const auto* transfer_status = value->GetDict().FindString("transfer_status");
  if (!transfer_status || transfer_status->empty()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  return base::unexpected(*transfer_status == "SESSION_TIME_OUT"
                              ? Error::kAccessTokenExpired
                              : Error::kUnexpectedError);
}

}  // namespace

// static
Result PostCommitTransactionBitFlyer::ProcessResponse(
    const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return {};
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      BLOG(0, "Access token expired!");
      return base::unexpected(Error::kAccessTokenExpired);
    case net::HTTP_CONFLICT:  // HTTP 409
      return ParseBody(response.body);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

absl::optional<std::string> PostCommitTransactionBitFlyer::Url() const {
  return endpoint::bitflyer::GetServerUrl(
      "/api/link/v1/coin/withdraw-to-deposit-id/request");
}

absl::optional<std::vector<std::string>> PostCommitTransactionBitFlyer::Headers(
    const std::string&) const {
  return endpoint::bitflyer::RequestAuthorization(token_);
}

absl::optional<std::string> PostCommitTransactionBitFlyer::Content() const {
  base::Value::Dict payload;
  payload.Set("currency_code", "BAT");
  payload.Set("amount", transaction_->amount);
  payload.Set("dry_run", false);
  payload.Set("deposit_id", transaction_->destination);
  payload.Set("transfer_id", transaction_->transaction_id);

  std::string json;
  if (!base::JSONWriter::Write(payload, &json)) {
    return absl::nullopt;
  }

  return json;
}

std::string PostCommitTransactionBitFlyer::ContentType() const {
  return kApplicationJson;
}

}  // namespace brave_rewards::internal::endpoints

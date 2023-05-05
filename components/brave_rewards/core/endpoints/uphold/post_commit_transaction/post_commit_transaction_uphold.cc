/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/post_commit_transaction/post_commit_transaction_uphold.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostCommitTransactionUphold::Error;
using Result = PostCommitTransactionUphold::Result;

namespace {

Result ParseBody(const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  const auto* status = value->GetDict().FindString("status");
  if (!status || status->empty()) {
    BLOG(0, "Failed to parse body!");
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
    const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(response.body);
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      BLOG(0, "Access token expired!");
      return base::unexpected(Error::kAccessTokenExpired);
    case net::HTTP_NOT_FOUND:  // HTTP 404
      BLOG(0, "Transaction not found!");
      return base::unexpected(Error::kTransactionNotFound);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

absl::optional<std::string> PostCommitTransactionUphold::Url() const {
  return endpoint::uphold::GetServerUrl(base::StringPrintf(
      "/v0/me/cards/%s/transactions/%s/commit", address_.c_str(),
      transaction_->transaction_id.c_str()));
}

absl::optional<std::vector<std::string>> PostCommitTransactionUphold::Headers(
    const std::string&) const {
  return endpoint::uphold::RequestAuthorization(token_);
}

}  // namespace brave_rewards::internal::endpoints

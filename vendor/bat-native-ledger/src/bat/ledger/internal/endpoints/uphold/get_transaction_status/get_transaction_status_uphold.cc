/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoints/uphold/get_transaction_status/get_transaction_status_uphold.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger::endpoints {
using Error = GetTransactionStatusUphold::Error;
using Result = GetTransactionStatusUphold::Result;

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

  return *status == "completed";
}

}  // namespace

// static
Result GetTransactionStatusUphold::ProcessResponse(
    const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(response.body);
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      BLOG(0, "Access token expired!");
      return base::unexpected(Error::kAccessTokenExpired);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

GetTransactionStatusUphold::GetTransactionStatusUphold(
    LedgerImpl* ledger,
    std::string&& token,
    std::string&& transaction_id)
    : RequestBuilder(ledger),
      token_(std::move(token)),
      transaction_id_(std::move(transaction_id)) {}

GetTransactionStatusUphold::~GetTransactionStatusUphold() = default;

absl::optional<std::string> GetTransactionStatusUphold::Url() const {
  return endpoint::uphold::GetServerUrl(
      base::StringPrintf("/v0/me/transactions/%s", transaction_id_.c_str()));
}

mojom::UrlMethod GetTransactionStatusUphold::Method() const {
  return mojom::UrlMethod::GET;
}

absl::optional<std::vector<std::string>> GetTransactionStatusUphold::Headers(
    const std::string&) const {
  return endpoint::uphold::RequestAuthorization(token_);
}

}  // namespace ledger::endpoints

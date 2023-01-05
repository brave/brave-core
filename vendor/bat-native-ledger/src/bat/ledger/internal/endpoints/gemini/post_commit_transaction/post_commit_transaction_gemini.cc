/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoints/gemini/post_commit_transaction/post_commit_transaction_gemini.h"

#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger::endpoints {
using Error = PostCommitTransactionGemini::Error;
using Result = PostCommitTransactionGemini::Result;

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

  if (*status == "Pending") {
    return base::unexpected(Error::kTransactionPending);
  }

  if (*status != "Completed") {
    return base::unexpected(Error::kUnexpectedError);
  }

  return {};
}

}  // namespace

PostCommitTransactionGemini::PostCommitTransactionGemini(
    LedgerImpl* ledger,
    std::string&& token,
    std::string&& address,
    std::string&& transaction_id,
    std::string&& destination,
    double amount)
    : PostCommitTransaction(ledger,
                            std::move(token),
                            std::move(address),
                            std::move(transaction_id)),
      destination_(std::move(destination)),
      amount_(amount) {}

// static
Result PostCommitTransactionGemini::ProcessResponse(
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

absl::optional<std::string> PostCommitTransactionGemini::Url() const {
  return endpoint::gemini::GetApiServerUrl("/v1/payments/pay");
}

absl::optional<std::vector<std::string>> PostCommitTransactionGemini::Headers(
    const std::string&) const {
  base::Value::Dict payload;
  payload.Set("tx_ref", transaction_id_);
  payload.Set("amount", base::StringPrintf("%f", amount_));
  payload.Set("currency", "BAT");
  payload.Set("destination", destination_);

  std::string json;
  if (!base::JSONWriter::Write(payload, &json)) {
    return absl::nullopt;
  }

  std::string base64;
  base::Base64Encode(json, &base64);

  auto headers = endpoint::gemini::RequestAuthorization(token_);
  headers.push_back("X-GEMINI-PAYLOAD: " + base64);
  return headers;
}

std::string PostCommitTransactionGemini::ContentType() const {
  return kApplicationJson;
}

}  // namespace ledger::endpoints

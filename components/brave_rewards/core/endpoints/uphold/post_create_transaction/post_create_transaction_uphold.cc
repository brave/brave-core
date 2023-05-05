/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/uphold/post_create_transaction/post_create_transaction_uphold.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = PostCreateTransactionUphold::Error;
using Result = PostCreateTransactionUphold::Result;

namespace {

Result ParseBody(const std::string& body) {
  auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  auto* id = value->GetDict().FindString("id");
  if (!id || id->empty()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  return std::move(*id);
}

}  // namespace

// static
Result PostCreateTransactionUphold::ProcessResponse(
    const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_ACCEPTED:  // HTTP 202
      return ParseBody(response.body);
    case net::HTTP_UNAUTHORIZED:  // HTTP 401
      BLOG(0, "Access token expired!");
      return base::unexpected(Error::kAccessTokenExpired);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

absl::optional<std::string> PostCreateTransactionUphold::Url() const {
  return endpoint::uphold::GetServerUrl(
      base::StringPrintf("/v0/me/cards/%s/transactions", address_.c_str()));
}

absl::optional<std::vector<std::string>> PostCreateTransactionUphold::Headers(
    const std::string&) const {
  return endpoint::uphold::RequestAuthorization(token_);
}

absl::optional<std::string> PostCreateTransactionUphold::Content() const {
  base::Value::Dict denomination;
  denomination.Set("amount", transaction_->amount);
  denomination.Set("currency", "BAT");

  base::Value::Dict payload;
  payload.Set("destination", transaction_->destination);
  payload.Set("denomination", std::move(denomination));
  if (transaction_->destination == uphold::GetFeeAddress()) {
    payload.Set("message", kFeeMessage);
  }

  std::string json;
  if (!base::JSONWriter::Write(payload, &json)) {
    return absl::nullopt;
  }

  return json;
}

}  // namespace brave_rewards::internal::endpoints

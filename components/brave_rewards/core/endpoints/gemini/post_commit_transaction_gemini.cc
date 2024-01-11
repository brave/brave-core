/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/gemini/post_commit_transaction_gemini.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"

namespace brave_rewards::internal::endpoints {
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

std::optional<std::string> PostCommitTransactionGemini::Url() const {
  return GURL(engine_->Get<EnvironmentConfig>().gemini_api_url())
      .Resolve("/v1/payments/pay")
      .spec();
}

std::optional<std::vector<std::string>> PostCommitTransactionGemini::Headers(
    const std::string&) const {
  base::Value::Dict payload;
  payload.Set("tx_ref", transaction_->transaction_id);
  payload.Set("amount", transaction_->amount);
  payload.Set("currency", "BAT");
  payload.Set("destination", transaction_->destination);

  std::string json;
  if (!base::JSONWriter::Write(payload, &json)) {
    return std::nullopt;
  }

  std::string base64;
  base::Base64Encode(json, &base64);

  return std::vector<std::string>{"Authorization: Bearer " + token_,
                                  "X-GEMINI-PAYLOAD: " + base64};
}

std::string PostCommitTransactionGemini::ContentType() const {
  return kApplicationJson;
}

}  // namespace brave_rewards::internal::endpoints

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_suggestions_claim/post_suggestions_claim.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/security_helper.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

// POST /v1/suggestions/claim
//
// Request body:
// {
//   "credentials": credentials": [
//     {
//       "t": "",
//       "publicKey": "",
//       "signature": ""
//     }
//   ],
//   "paymentId": "83b3b77b-e7c3-455b-adda-e476fa0656d2"
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_SERVICE_UNAVAILABLE (503)
//
// Response body:
// {Empty}

namespace ledger {
namespace endpoint {
namespace promotion {

PostSuggestionsClaim::PostSuggestionsClaim(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostSuggestionsClaim::~PostSuggestionsClaim() = default;

std::string PostSuggestionsClaim::GetUrl() {
  return GetServerUrl("/v1/suggestions/claim");
}

std::string PostSuggestionsClaim::GeneratePayload(
    const braveledger_credentials::CredentialsRedeem& redeem) {
  const std::string payment_id = ledger_->state()->GetPaymentId();
  base::Value credentials(base::Value::Type::LIST);
  braveledger_credentials::GenerateCredentials(
      redeem.token_list,
      payment_id,
      &credentials);

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", payment_id);
  body.SetKey("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(body, &json);
  return json;
}

ledger::Result PostSuggestionsClaim::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return ledger::Result::BAD_REGISTRATION_RESPONSE;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

void PostSuggestionsClaim::Request(
    const braveledger_credentials::CredentialsRedeem& redeem,
    PostSuggestionsClaimCallback callback) {
  auto url_callback = std::bind(&PostSuggestionsClaim::OnRequest,
      this,
      _1,
      callback);

  const std::string payload = GeneratePayload(redeem);

  auto headers = braveledger_request_util::BuildSignHeaders(
      "post /v1/suggestions/claim",
      payload,
      ledger_->state()->GetPaymentId(),
      ledger_->state()->GetRecoverySeed());

  ledger_->LoadURL(
      GetUrl(),
      headers,
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void PostSuggestionsClaim::OnRequest(
    const ledger::UrlResponse& response,
    PostSuggestionsClaimCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

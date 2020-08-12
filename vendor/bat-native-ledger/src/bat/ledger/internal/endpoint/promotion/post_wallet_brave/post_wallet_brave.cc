/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_wallet_brave/post_wallet_brave.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/security_helper.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

// POST /v3/wallet/brave
//
// Request body:
// {Empty}
//
// Success code:
// HTTP_CREATED (201)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_SERVICE_UNAVAILABLE (503)
//
// Response body:
// {
//  "paymentId": "37742974-3b80-461a-acfb-937e105e5af4"
// }

namespace ledger {
namespace endpoint {
namespace promotion {

PostWalletBrave::PostWalletBrave(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostWalletBrave::~PostWalletBrave() = default;

std::string PostWalletBrave::GetUrl() {
  return GetServerUrl("/v3/wallet/brave");
}

ledger::Result PostWalletBrave::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return ledger::Result::BAD_REGISTRATION_RESPONSE;
  }

  if (status_code != net::HTTP_CREATED) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result PostWalletBrave::ParseBody(
    const std::string& body,
    std::string* payment_id) {
  DCHECK(payment_id);

  base::Optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* payment_id_string = dictionary->FindStringKey("paymentId");
  if (!payment_id_string || payment_id_string->empty()) {
    BLOG(1, "Payment id is wrong");
    return ledger::Result::LEDGER_ERROR;
  }

  *payment_id = *payment_id_string;

  return ledger::Result::LEDGER_OK;
}

void PostWalletBrave::Request(
    PostWalletBraveCallback callback) {
  const auto seed = ledger_->state()->GetRecoverySeed();
  const auto headers = braveledger_request_util::BuildSignHeaders(
      "post /v3/wallet/brave",
      "",
      braveledger_helper::Security::GetPublicKeyHexFromSeed(seed),
      seed);

  auto url_callback = std::bind(&PostWalletBrave::OnRequest,
      this,
      _1,
      callback);

  ledger_->LoadURL(
      GetUrl(),
      headers,
      "",
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void PostWalletBrave::OnRequest(
    const ledger::UrlResponse& response,
    PostWalletBraveCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  std::string payment_id;
  ledger::Result result = CheckStatusCode(response.status_code);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, payment_id);
    return;
  }

  result = ParseBody(response.body, &payment_id);
  callback(result, payment_id);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

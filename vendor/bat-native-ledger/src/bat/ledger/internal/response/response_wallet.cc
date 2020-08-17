/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/response/response_wallet.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/logging.h"
#include "net/http/http_status_code.h"

namespace braveledger_response_util {

// Request Url:
// POST /v3/wallet/brave
//
// Success:
// OK (201)
//
// Response Format:
// {
//  "paymentId": "37742974-3b80-461a-acfb-937e105e5af4",
//  "walletProvider": {
//    "id": "",
//    "name": "brave"
//  },
//  "altcurrency": "BAT",
//  "publicKey": "90035db3b131044c7c845bfa987946258ef4dc947ba"
// }

ledger::Result ParseCreateWallet(
    const ledger::UrlResponse& response,
    std::string* payment_id) {
  DCHECK(payment_id);

  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Service Unavailable (503)
  if (response.status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return ledger::Result::BAD_REGISTRATION_RESPONSE;
  }

  if (response.status_code != net::HTTP_CREATED) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
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

// Request Url:
// GET /v3/wallet/recover/{public_key}
//
// Success:
// OK (200)
//
// Response Format:
// {
//  "paymentId": "d59d4b69-f66e-4ee8-9c88-1c522e02ffd3",
//  "walletProvider": {
//    "id": "a9d12d76-2b6d-4f8b-99df-bb801bff9407",
//    "name": "uphold"
//  },
//  "altcurrency": "BAT",
//  "publicKey": "79d7da2a756cc8d9403d0353a64fae5698e01b44a2c2745"
// }

ledger::Result ParseRecoverWallet(
    const ledger::UrlResponse& response,
    std::string* payment_id) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Not found");
    return ledger::Result::NOT_FOUND;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
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
    BLOG(0, "Payment id is missing");
    return ledger::Result::LEDGER_ERROR;
  }

  *payment_id = *payment_id_string;
  return ledger::Result::LEDGER_OK;
}

// Request Url:
// POST /v3/wallet/uphold/{payment_id}/claim
//
// Success:
// OK (200)
//
// Response Format:
// {Empty body}

ledger::Result CheckClaimWallet(const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Forbidden (403)
  if (response.status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Forbidden");
    return ledger::Result::NOT_FOUND;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Not found");
    return ledger::Result::LEDGER_ERROR;
  }

  // Conflict (409)
  if (response.status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Not found");
    return ledger::Result::ALREADY_EXISTS;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// GET /v3/wallet/uphold/{payment_id}
//
// Success:
// OK (200)
//
// Response Format:
// {
//  "total": 0.0
//  "spendable": 0.0
//  "confirmed": 0.0
//  "unconfirmed": 0.0
// }

ledger::BalancePtr ParseWalletBalance(const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid payment id");
    return nullptr;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized payment id");
    return nullptr;
  }

  // Service Unavailable (503)
  if (response.status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return nullptr;
  }

  if (response.status_code != net::HTTP_OK) {
    return nullptr;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return nullptr;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return nullptr;
  }

  ledger::BalancePtr balance = ledger::Balance::New();

  const auto confirmed = dictionary->FindDoubleKey("confirmed");
  if (confirmed) {
    balance->total = *confirmed;
  }

  balance->user_funds = balance->total;
  balance->wallets.insert(
      std::make_pair(ledger::kWalletAnonymous, balance->total));

  return balance;
}

}  // namespace braveledger_response_util

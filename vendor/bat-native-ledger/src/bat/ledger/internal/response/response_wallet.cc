/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/response/response_wallet.h"

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
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
// OK (201)
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

  if (response.status_code != net::HTTP_CREATED) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// GET /wallet/{payment_id}/balance
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "altcurrency": "BAT",
//   "probi": "0",
//   "cardBalance": "0",
//   "balance": "0.0000",
//   "unconfirmed": "0.0000",
//   "rates": {
//     "AED": 0.864759348,
//     "ARS": 16.3201969584,
//     "AUD": 0.3420872568,
//     "BAT": 1,
//     "BCH": 0.00096277302,
//     "BRL": 1.17166716,
//     "BTC": 0.0000247753512,
//     "BTG": 0.0267530566176,
//     "CAD": 0.3193696512,
//     "CHF": 0.2209298328,
//     "CNY": 1.663407144,
//     "DASH": 0.0030994899048,
//     "DKK": 1.5428430288,
//     "ETH": 0.000987728323025758,
//     "EUR": 0.2069541144,
//     "GBP": 0.1864519992,
//     "HKD": 1.8247023792,
//     "ILS": 0.8132238864,
//     "INR": 17.927590572,
//     "JPY": 25.1176738968,
//     "KES": 25.0625197224,
//     "LBA": 19.393739703459637,
//     "LTC": 0.0052345069128,
//     "MXN": 5.3096381712,
//     "NOK": 2.2435171776,
//     "NZD": 0.3647860272,
//     "PHP": 11.8485109368,
//     "PLN": 0.926880192,
//     "SEK": 2.178867708,
//     "SGD": 0.3270167424,
//     "USD": 0.23624649565116654,
//     "XAG": 0.0128360428272,
//     "XAU": 0.00013620204,
//     "XPD": 0.0001530524808,
//     "XPT": 0.0002889696384,
//     "XRP": 1.21298311296
//   },
//   "parameters": {
//     "adFree": {
//       "currency": "BAT",
//       "fee": {
//         "BAT": 10
//       },
//       "choices": {
//         "BAT": [
//           5,
//           10,
//           15,
//           20,
//           25,
//           50,
//           100
//         ]
//       },
//       "range": {
//         "BAT": [
//           5,
//           100
//         ]
//       },
//       "days": 30
//     },
//     "defaultTipChoices": [
//       "1",
//       "10",
//       "50"
//     ],
//     "defaultMonthlyChoices": [
//       "1",
//       "10",
//       "50"
//     ]
//   }
// }

ledger::BalancePtr ParseWalletFetchBalance(
    const ledger::UrlResponse& response) {
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

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
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
    return nullptr;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return nullptr;
  }

  ledger::BalancePtr balance = ledger::Balance::New();

  double total_anon = 0.0;
  const auto* total = dictionary->FindStringKey("balance");
  if (total) {
    const bool success = base::StringToDouble(*total, &total_anon);
    if (!success) {
      total_anon = 0.0;
    }
  }
  balance->total = total_anon;

  std::string user_funds = "0";
  const auto* funds = dictionary->FindStringKey("cardBalance");
  if (funds) {
    user_funds = *funds;
  }
  balance->user_funds = user_funds;

  return balance;
}

}  // namespace braveledger_response_util

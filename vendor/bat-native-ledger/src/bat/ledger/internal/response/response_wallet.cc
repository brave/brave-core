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

// Request Url:
// GET /v2/wallet?publicKey={public_key_hex}
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "paymentId": <string>
// }

ledger::Result ParseWalletRecoverKey(
    const ledger::UrlResponse& response,
    std::string* payment_id) {
  DCHECK(payment_id);

  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Required query parameter 'publicKey' must use hex format");
    return ledger::Result::LEDGER_ERROR;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized payment id");
    return ledger::Result::LEDGER_ERROR;
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

  const auto* payment_id_str = dictionary->FindStringKey("paymentId");
  if (!payment_id_str || payment_id_str->empty()) {
    BLOG(0, "Missing payment id");
    return ledger::Result::LEDGER_ERROR;
  }

  *payment_id = *payment_id_str;

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// GET /v2/wallet/{recovery_id}
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "altcurrency": "BAT",
//   "paymentStamp": 0,
//   "httpSigningPubKey":
//   "84b6a82c717e108c40f6ff18fa09e85d3993c4ed758a41053949a0e8da9f25a8",
//   "rates": {
//     "AED": 0.905958048,
//     "ARS": 17.081127192,
//     "AUD": 0.3522734456,
//     "BAT": 1,
//     "BCH": 0.0009752786864,
//     "BRL": 1.213567456,
//     "BTC": 0.0000252978648,
//     "BTG": 0.026993897092,
//     "CAD": 0.33037428,
//     "CHF": 0.2331956536,
//     "CNY": 1.7415299728,
//     "DASH": 0.0031886679752,
//     "DKK": 1.6197490064,
//     "ETH": 0.001015345366037685,
//     "EUR": 0.2172355792,
//     "GBP": 0.193402756,
//     "HKD": 1.911496996,
//     "ILS": 0.8474698384,
//     "INR": 18.6620156,
//     "JPY": 26.45546964,
//     "KES": 26.2795314624,
//     "LBA": 19.075019334880125,
//     "LTC": 0.0053489630712,
//     "MXN": 5.40715038,
//     "NOK": 2.301841792,
//     "NZD": 0.3769695088,
//     "PHP": 12.32719052,
//     "PLN": 0.9668805944,
//     "SEK": 2.2717344472,
//     "SGD": 0.341497744,
//     "USD": 0.2470464500588899,
//     "XAG": 0.0135806002016,
//     "XAU": 0.0001447135536,
//     "XPD": 0.0001603332648,
//     "XPT": 0.0003027160704,
//     "XRP": 1.22896001176
//   },
//   "addresses": {
//     "BAT": "0x51DA9199BE5dc5Cd322DF866a46Bf0B606f1C809",
//     "BTC": "mqnyP8CCY2bNNr2etDuQGK8aMdtLiZgTqN",
//     "CARD_ID": "1c8dbc44-1851-4c9e-96f4-b35d41f2984d",
//     "ETH": "0x51DA9199BE5dc5Cd322DF866a46Bf0B606f1C809",
//     "LTC": "myTdNu6wcMLrkok2WMq1HXPZRKPX27avUu"
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
//   },
//   "balance": "0.0000",
//   "cardBalance": "0",
//   "probi": "0",
//   "unconfirmed": "0.0000"
// }

ledger::Result ParseRecoverWallet(
    const ledger::UrlResponse& response,
    std::string* card_id,
    double* balance) {
  DCHECK(card_id && balance);

  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized currency or payment id");
    return ledger::Result::LEDGER_ERROR;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  // Service Unavailable (503)
  if (response.status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return ledger::Result::LEDGER_ERROR;
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

  const auto* balance_str = dictionary->FindStringKey("balance");
  if (!balance_str) {
    BLOG(0, "Missing balance");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* card_id_str = dictionary->FindStringPath("addresses.CARD_ID");
  if (!card_id_str || card_id_str->empty()) {
    BLOG(0, "Missing card id");
    return ledger::Result::LEDGER_ERROR;
  }

  const bool success = base::StringToDouble(*balance_str, balance);
  if (!success) {
    *balance = 0.0;
  }

  *card_id = *card_id_str;

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// POST /v2/registrar/persona/{user_id}
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "wallet": {
//     "paymentId": "f60c8c19-a7ef-4e3f-9810-137b7e0313bf",
//     "addresses": {
//       "BAT": "0x32B38C1459E86ced1B1B7e6c60FCdd309530f790",
//       "BTC": "mngKw1NC5R3NEmCJAf5FuaSEXNDubBkM2t",
//       "CARD_ID": "a698d76b-f5aa-42a4-bf2d-8994563c7f2f",
//       "ETH": "0x32B38C1459E86ced1B1B7e6c60FCdd309530f790",
//       "LTC": "mtD1hALT2ZjBjidTJVmR3W6nt569bEPRpP"
//     }
//   },
//   "payload": {
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
//   },
//   "verification": "315GcEJ/VBfh3J/NEy1tYMM7hsGorjq+ACBxdOslroH 9d+yn7vqSow\n"
// }

ledger::Result ParseWalletRegisterPersona(
    const ledger::UrlResponse& response,
    std::string* payment_id,
    std::string* card_id) {
  DCHECK(payment_id && card_id);

  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized registrar");
    return ledger::Result::NOT_FOUND;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
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

  const auto* payment_id_string =
      dictionary->FindStringPath("wallet.paymentId");
  if (!payment_id_string || payment_id_string->empty()) {
    BLOG(0, "Missing payment id");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* card_id_string =
      dictionary->FindStringPath("wallet.addresses.CARD_ID");
  if (!card_id_string || card_id_string->empty()) {
    BLOG(0, "Missing card id");
    return ledger::Result::LEDGER_ERROR;
  }

  *payment_id = *payment_id_string;
  *card_id = *card_id_string;

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// GET /v2/registrar/persona
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "payload": {
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
//   },
//   "registrarVK": "==========ANONLOGIN_VK_BEG======= ..."
// }

ledger::Result CheckWalletRequestCredentials(
  const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid registrar type");
    return ledger::Result::LEDGER_ERROR;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized registrar");
    return ledger::Result::LEDGER_ERROR;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::BAD_REGISTRATION_RESPONSE;
  }

  return ledger::Result::LEDGER_OK;
}

// Request Url:
// GET /v2/wallet/{payment_id}/claim
//
// Success:
// OK (200)
//
// Format Response (error):
// {
//   "statusCode": 500,
//   "error": "Internal Server Error",
//   "message": "An internal server error occurred"
// }

ledger::Result CheckTransferAnonToExternalWallet(
    const ledger::UrlResponse& response) {
  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Forbidden (403)
  if (response.status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Invalid card or providerLinkingId");
    return ledger::Result::LEDGER_ERROR;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized payment id");
    return ledger::Result::LEDGER_ERROR;
  }

  // Conflict (409)
  if (response.status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Maximum member associations or other conflict");
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

}  // namespace braveledger_response_util

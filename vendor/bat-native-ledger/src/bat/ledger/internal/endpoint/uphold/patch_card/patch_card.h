/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_UPHOLD_PATCH_CARD_PATCH_CARD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_UPHOLD_PATCH_CARD_PATCH_CARD_H_

#include <string>

#include "bat/ledger/ledger.h"

// PATCH https://api.uphold.com/v0/me/cards/{wallet_address}
//
// Request body:
// {
//   "label": "Brave Browser",
//   "settings": {
//     "position": -1,
//     "starred": true
//   }
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "CreatedByApplicationId": "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
//   "address": {
//     "wire": "XXXXXXXXXX"
//   },
//   "available": "0.00",
//   "balance": "0.00",
//   "currency": "BAT",
//   "id": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
//   "label": "Brave Browser",
//   "lastTransactionAt": null,
//   "settings": {
//     "position": 8,
//     "protected": false,
//     "starred": false
//   },
//   "createdByApplicationClientId": "4c2b665ca060d912fec5c735c734859a06118cc8",
//   "normalized": [
//     {
//       "available": "0.00",
//       "balance": "0.00",
//       "currency": "USD"
//     }
//   ],
//   "wire": [
//     {
//       "accountName": "Uphold Europe Limited",
//       "address": {
//         "line1": "Tartu mnt 2",
//         "line2": "10145 Tallinn, Estonia"
//       },
//       "bic": "LHVBEE22",
//       "currency": "EUR",
//       "iban": "EE76 7700 7710 0159 0178",
//       "name": "AS LHV Pank"
//     },
//     {
//       "accountName": "Uphold HQ, Inc.",
//       "accountNumber": "XXXXXXXXXX",
//       "address": {
//         "line1": "1359 Broadway",
//         "line2": "New York, NY 10018"
//       },
//       "bic": "MCBEUS33",
//       "currency": "USD",
//       "name": "Metropolitan Bank",
//       "routingNumber": "XXXXXXXXX"
//     }
//   ]
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace uphold {

using PatchCardCallback = std::function<void(const type::Result result)>;

class PatchCard {
 public:
  explicit PatchCard(LedgerImpl* ledger);
  ~PatchCard();

  void Request(const std::string& token,
               const std::string& address,
               PatchCardCallback callback);

 private:
  std::string GetUrl(const std::string& address);

  std::string GeneratePayload();

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(const type::UrlResponse& response, PatchCardCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_UPHOLD_PATCH_CARD_PATCH_CARD_H_

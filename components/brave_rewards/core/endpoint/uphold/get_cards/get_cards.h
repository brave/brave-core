/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_UPHOLD_GET_CARDS_GET_CARDS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_UPHOLD_GET_CARDS_GET_CARDS_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

// GET https://api.uphold.com/v0/me/cards?q=currency:BAT
//
// Success codes:
// HTTP_OK (200)
// HTTP_PARTIAL_CONTENT (206)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// [
//   {
//     "CreatedByApplicationId": null,
//     "address": {
//       "wire": "XXXXXXXXXX"
//     },
//     "available": "12.35",
//     "balance": "12.35",
//     "currency": "BAT",
//     "id": "3ed3b2c4-a715-4c01-b302-fa2681a971ea",
//     "label": "Twitter - User - Brave Rewards",
//     "lastTransactionAt": "2020-03-31T19:27:57.552Z",
//     "settings": {
//       "position": 7,
//       "protected": false,
//       "starred": true
//     },
//     "normalized": [
//       {
//         "available": "3.15",
//         "balance": "3.15",
//         "currency": "USD"
//       }
//     ],
//     "wire": [
//       {
//         "accountName": "Uphold Europe Limited",
//         "address": {
//           "line1": "Tartu mnt 2",
//           "line2": "10145 Tallinn, Estonia"
//         },
//         "bic": "LHVBEE22",
//         "currency": "EUR",
//         "iban": "EE76 7700 7710 0159 0178",
//         "name": "AS LHV Pank"
//       },
//       {
//         "accountName": "Uphold HQ, Inc.",
//         "accountNumber": "XXXXXXXXXX",
//         "address": {
//           "line1": "1359 Broadway",
//           "line2": "New York, NY 10018"
//         },
//         "bic": "MCBEUS33",
//         "currency": "USD",
//         "name": "Metropolitan Bank",
//         "routingNumber": "XXXXXXXXX"
//       }
//     ]
//   }
// ]

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoint::uphold {

using GetCardsCallback =
    base::OnceCallback<void(mojom::Result, std::string&& id)>;

class GetCards {
 public:
  explicit GetCards(RewardsEngine& engine);
  ~GetCards();

  void Request(const std::string& token, GetCardsCallback) const;

 private:
  std::string GetUrl() const;

  mojom::Result CheckStatusCode(int status_code) const;

  mojom::Result ParseBody(const std::string& body, std::string* id) const;

  void OnRequest(GetCardsCallback, mojom::UrlResponsePtr) const;

  const raw_ref<RewardsEngine> engine_;
};

}  // namespace endpoint::uphold
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_UPHOLD_GET_CARDS_GET_CARDS_H_

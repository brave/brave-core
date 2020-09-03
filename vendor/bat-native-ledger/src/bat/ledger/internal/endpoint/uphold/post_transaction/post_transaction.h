/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_UPHOLD_POST_TRANSACTION_POST_TRANSACTION_H_
#define BRAVELEDGER_ENDPOINT_UPHOLD_POST_TRANSACTION_POST_TRANSACTION_H_

#include <string>

#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/ledger.h"

// POST https://api.uphold.com/v0/me/cards/{wallet_address}/transactions
//
// Request body:
// {
//   "denomination": {
//     "amount": 1.0,
//     "currency: "BAT
//   }
//   "destination": "f5e37294-68f1-49ae-89e2-b24b64aedd37",
//   "message": "Hi"
// }
//
// Success code:
// HTTP_ACCEPTED (202)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "createdAt": "2020-06-10T18:58:21.683Z",
//   "denomination": {
//     "amount": "1.00",
//     "currency": "BAT",
//     "pair": "BATBAT",
//     "rate": "1.00"
//   },
//   "fees": [],
//   "id": "d382d3ae-8462-4b2c-9b60-b669539f41b2",
//   "network": "uphold",
//   "normalized": [
//     {
//       "commission": "0.00",
//       "currency": "USD",
//       "fee": "0.00",
//       "rate": "0.24688",
//       "target": "origin",
//       "amount": "0.25"
//     }
//   ],
//   "params": {
//     "currency": "BAT",
//     "margin": "0.00",
//     "pair": "BATBAT",
//     "rate": "1.00",
//     "ttl": 3599588,
//     "type": "internal"
//   },
//   "priority": "normal",
//   "status": "pending",
//   "type": "transfer",
//   "destination": {
//     "amount": "1.00",
//     "base": "1.00",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "Brave Software International",
//     "fee": "0.00",
//     "isMember": true,
//     "node": {
//       "id": "6654ecb0-6079-4f6c-ba58-791cc890a561",
//       "type": "card",
//       "user": {
//         "id": "f5e37294-68f1-49ae-89e2-b24b64aedd37",
//         "username": "braveintl"
//       }
//     },
//     "rate": "1.00",
//     "type": "card",
//     "username": "braveintl"
//   },
//   "origin": {
//     "amount": "1.00",
//     "base": "1.00",
//     "CardId": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "User",
//     "fee": "0.00",
//     "isMember": true,
//     "node": {
//       "id": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
//       "type": "card",
//       "user": {
//         "id": "b34060c9-5ca3-4bdb-bc32-1f826ecea36e"
//       }
//     },
//     "rate": "1.00",
//     "sources": [],
//     "type": "card"
//   }
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace uphold {

using PostTransactionCallback = std::function<void(
    const type::Result result,
    const std::string& id)>;

class PostTransaction {
 public:
  explicit PostTransaction(LedgerImpl* ledger);
  ~PostTransaction();

  void Request(
      const std::string& token,
      const std::string& address,
      const ::ledger::uphold::Transaction& transaction,
      PostTransactionCallback callback);

 private:
  std::string GetUrl(const std::string& address);

  std::string GeneratePayload(
      const ::ledger::uphold::Transaction& transaction);

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(
      const std::string& body,
      std::string* id);

  void OnRequest(
      const type::UrlResponse& response,
      PostTransactionCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_UPHOLD_POST_TRANSACTION_POST_TRANSACTION_H_

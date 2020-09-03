/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_UPHOLD_POST_TRANSACTION_COMMIT_\
POST_TRANSACTION_COMMIT_H_
#define BRAVELEDGER_ENDPOINT_UPHOLD_POST_TRANSACTION_COMMIT_\
POST_TRANSACTION_COMMIT_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST https://api.uphold.com/v0/me/cards/{wallet_address}/transactions/{transaction_id}/commit //NOLINT
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "application": {
//     "name": "Brave Browser"
//   },
//   "createdAt": "2020-06-10T18:58:22.351Z",
//   "denomination": {
//     "pair": "BATBAT",
//     "rate": "1.00",
//     "amount": "1.00",
//     "currency": "BAT"
//   },
//   "fees": [],
//   "id": "d382d3ae-8462-4b2c-9b60-b669539f41b2",
//   "message": null,
//   "network": "uphold",
//   "normalized": [
//     {
//       "fee": "0.00",
//       "rate": "0.24688",
//       "amount": "0.25",
//       "target": "origin",
//       "currency": "USD",
//       "commission": "0.00"
//     }
//   ],
//   "params": {
//     "currency": "BAT",
//     "margin": "0.00",
//     "pair": "BATBAT",
//     "progress": "1",
//     "rate": "1.00",
//     "ttl": 3599588,
//     "type": "internal"
//   },
//   "priority": "normal",
//   "reference": null,
//   "status": "completed",
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
//     "sources": [
//       {
//         "id": "463dca02-83ec-4bd6-93b0-73bf5dbe35ac",
//         "amount": "1.00"
//       }
//     ],
//     "type": "card"
//   }
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace uphold {

using PostTransactionCommitCallback = std::function<void(
    const type::Result result)>;

class PostTransactionCommit {
 public:
  explicit PostTransactionCommit(LedgerImpl* ledger);
  ~PostTransactionCommit();

  void Request(
      const std::string& token,
      const std::string& address,
      const std::string& transaction_id,
      PostTransactionCommitCallback callback);

 private:
  std::string GetUrl(
    const std::string& address,
    const std::string& transaction_id);

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(
      const type::UrlResponse& response,
      PostTransactionCommitCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_UPHOLD_POST_TRANSACTION_COMMIT_\
// POST_TRANSACTION_COMMIT_H_

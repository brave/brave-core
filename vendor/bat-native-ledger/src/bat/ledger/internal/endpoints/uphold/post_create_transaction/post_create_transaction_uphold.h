/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_UPHOLD_POST_CREATE_TRANSACTION_POST_CREATE_TRANSACTION_UPHOLD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_UPHOLD_POST_CREATE_TRANSACTION_POST_CREATE_TRANSACTION_UPHOLD_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/endpoints/post_create_transaction/post_create_transaction.h"
#include "bat/ledger/internal/endpoints/response_handler.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_endpoints.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// POST /v0/me/cards/:card-id/transactions
//
// Request body:
// {
//   "denomination": {
//     "amount": "0.050000",
//     "currency": "BAT"
//   },
//   "destination": "1b2b466f-5c15-49bf-995e-c91777d3da93",
//   "message": "5% transaction fee collected by Brave Software International"
// }
//
// Response body:
// {
//   "application": {
//     "clientId": "4c2b665ca060d912fec5c735c734859a06118cc8",
//     "name": "Brave Browser"
//   },
//   "createdAt": "2022-12-08T16:35:33.120Z",
//   "denomination": {
//     "amount": "0.05",
//     "currency": "BAT",
//     "pair": "BATBAT",
//     "rate": "1.00",
//     "target": "origin"
//   },
//   "destination": {
//     "amount": "0.05",
//     "base": "0.05",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "Brave Browser",
//     "fee": "0.00",
//     "node": {
//       "type": "anonymous"
//     },
//     "rate": "1.00",
//     "type": "anonymous"
//   },
//   "fees": [],
//   "id": "87725361-4245-4435-a75a-f7a85674714a",
//   "message": "5% transaction fee collected by Brave Software International",
//   "network": "uphold",
//   "normalized": [
//     {
//       "amount": "0.01",
//       "commission": "0.00",
//       "currency": "USD",
//       "fee": "0.00",
//       "rate": "0.22325468170000000000",
//       "target": "origin"
//     }
//   ],
//   "origin": {
//     "CardId": "2d3589a4-cb7b-41b9-8f23-9d716f2e6016",
//     "amount": "0.05",
//     "base": "0.05",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "description",
//     "fee": "0.00",
//     "isMember": true,
//     "node": {
//       "id": "5d3689f6-cbcb-42b7-8f33-7d716f2e7007",
//       "type": "card",
//       "user": {
//         "id": "bcc2b79a-b42c-418f-8d84-271d16bf5ff5"
//       }
//     },
//     "rate": "1.00",
//     "sources": [],
//     "type": "card"
//   },
//   "params": {
//     "currency": "BAT",
//     "margin": "0.00",
//     "pair": "BATBAT",
//     "rate": "1.00",
//     "ttl": 3599996,
//     "type": "internal"
//   },
//   "priority": "normal",
//   "status": "pending",
//   "type": "transfer"
// }

namespace ledger::endpoints {

class PostCreateTransactionUphold;

template <>
struct ResultFor<PostCreateTransactionUphold> {
  using Value = std::string;  // transaction ID
  using Error = mojom::PostCreateTransactionUpholdError;
};

class PostCreateTransactionUphold final
    : public PostCreateTransaction,
      public ResponseHandler<PostCreateTransactionUphold> {
 public:
  using PostCreateTransaction::PostCreateTransaction;

  static Result ProcessResponse(const mojom::UrlResponse&);

 private:
  absl::optional<std::string> Url() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
  absl::optional<std::string> Content() const override;
};

}  // namespace ledger::endpoints

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_UPHOLD_POST_CREATE_TRANSACTION_POST_CREATE_TRANSACTION_UPHOLD_H_

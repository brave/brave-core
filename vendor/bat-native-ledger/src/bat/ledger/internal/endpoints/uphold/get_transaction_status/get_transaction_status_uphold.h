/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_UPHOLD_GET_TRANSACTION_STATUS_GET_TRANSACTION_STATUS_UPHOLD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_UPHOLD_GET_TRANSACTION_STATUS_GET_TRANSACTION_STATUS_UPHOLD_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/endpoints/request_builder.h"
#include "bat/ledger/internal/endpoints/response_handler.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_endpoints.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// GET /v0/me/transactions/:transaction-id
//
// Request body:
// -
//
// Response body:
// {
//   "application": {
//     "clientId": "4c2b665ca060d912fec5c735c734859a06118cc8",
//     "name": "Brave Browser"
//   },
//   "createdAt": "2022-12-19T13:25:32.430Z",
//   "denomination": {
//     "amount": "0.95",
//     "currency": "BAT",
//     "pair": "BATBAT",
//     "rate": "1.00",
//     "target": "origin"
//   },
//   "destination": {
//     "amount": "0.95",
//     "base": "0.95",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "Uphold Member",
//     "fee": "0.00",
//     "node": {
//       "type": "anonymous"
//     },
//     "rate": "1.00",
//     "type": "anonymous"
//   },
//   "fees": [],
//   "id": "1423ac5e-85b5-44ad-0d9b-40c35dbd3376",
//   "message": null,
//   "network": "uphold",
//   "normalized": [
//     {
//       "amount": "0.18",
//       "commission": "0.00",
//       "currency": "USD",
//       "fee": "0.00",
//       "rate": "0.18851850220000000000",
//       "target": "origin"
//     }
//   ],
//   "origin": {
//     "CardId": "1e8429f4-cc7c-48b7-8f33-9d746f2e7576",
//     "amount": "0.95",
//     "base": "0.95",
//     "commission": "0.00",
//     "currency": "BAT",
//     "description": "Szilard Szaloki",
//     "fee": "0.00",
//     "isMember": true,
//     "node": {
//       "id": "1e8429f4-cc7c-48b7-8f33-9d746f2e7576",
//       "type": "card",
//       "user": {
//         "id": "bcc2b79a-b42c-418f-8d84-271d16bf5ff5"
//       }
//     },
//     "rate": "1.00",
//     "sources": [
//       {
//         "amount": "0.95",
//         "id": "0954af88-f2ba-10b4-8a8c-927eb98b9543"
//       }
//     ],
//     "type": "card"
//   },
//   "params": {
//     "currency": "BAT",
//     "margin": "0.00",
//     "pair": "BATBAT",
//     "progress": "1",
//     "rate": "1.00",
//     "ttl": 3599998,
//     "type": "internal"
//   },
//   "priority": "normal",
//   "reference": null,
//   "status": "completed",
//   "type": "transfer"
// }

namespace ledger {
class LedgerImpl;

namespace endpoints {

class GetTransactionStatusUphold;

template <>
struct ResultFor<GetTransactionStatusUphold> {
  using Value = bool;  // transaction completed
  using Error = mojom::GetTransactionStatusUpholdError;
};

class GetTransactionStatusUphold final
    : public RequestBuilder,
      public ResponseHandler<GetTransactionStatusUphold> {
 public:
  static Result ProcessResponse(const mojom::UrlResponse&);

  GetTransactionStatusUphold(LedgerImpl*,
                             std::string&& token,
                             std::string&& transaction_id);
  ~GetTransactionStatusUphold() override;

 private:
  std::string token_;
  std::string transaction_id_;

  absl::optional<std::string> Url() const override;
  mojom::UrlMethod Method() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
};

}  // namespace endpoints
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_UPHOLD_GET_TRANSACTION_STATUS_GET_TRANSACTION_STATUS_UPHOLD_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PAYMENT_POST_TRANSACTION_ANON_\
POST_TRANSACTION_ANON_H_
#define BRAVELEDGER_ENDPOINT_PAYMENT_POST_TRANSACTION_ANON_\
POST_TRANSACTION_ANON_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST /v1/orders/{order_id}/transactions/anonymousCard
//
// Request body:
// {
//   "paymentId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "kind": "anonymous-card",
//   "transaction": "base64_string"
// }
//
// Success code:
// HTTP_CREATED (201)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
// HTTP_CONFLICT (409)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {
//   "id": "80740e9c-08c3-43ed-92aa-2a7be8352000",
//   "orderId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "createdAt": "2020-06-10T18:58:22.817675Z",
//   "updatedAt": "2020-06-10T18:58:22.817675Z",
//   "external_transaction_id": "d382d3ae-8462-4b2c-9b60-b669539f41b2",
//   "status": "completed",
//   "currency": "BAT",
//   "kind": "uphold",
//   "amount": "1"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace payment {

using PostTransactionAnonCallback = std::function<void(
    const type::Result result)>;

class PostTransactionAnon {
 public:
  explicit PostTransactionAnon(LedgerImpl* ledger);
  ~PostTransactionAnon();

  void Request(
      const double amount,
      const std::string& order_id,
      const std::string& destination,
      PostTransactionAnonCallback callback);

 private:
  std::string GetUrl(const std::string& order_id);

  std::string GeneratePayload(
      const double amount,
      const std::string& order_id,
      const std::string& destination);

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(
      const type::UrlResponse& response,
      PostTransactionAnonCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PAYMENT_POST_TRANSACTION_ANON_\
// POST_TRANSACTION_ANON_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_ORDER_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_ORDER_ENDPOINT_H_

#include <map>
#include <string>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/url_fetcher.h"
#include "bat/ledger/internal/payments/payment_data.h"

// POST /v1/orders
//
// Request body:
// {
//   "items": [
//     {
//       "sku": "okasofkasofdkasofkoasdkf",
//       "quantity": 5
//     }
//   ]
// }
//
// Success code:
// HTTP_CREATED (201)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {
//   "id": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "createdAt": "2020-06-10T18:58:21.378752Z",
//   "currency": "BAT",
//   "updatedAt": "2020-06-10T18:58:21.378752Z",
//   "totalPrice": "1",
//   "location": "brave.com",
//   "status": "pending",
//   "items": [
//     {
//       "id": "9c9aed7f-b349-452e-80a8-95faf2b1600d",
//       "orderId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//       "sku": "user-wallet-vote",
//       "createdAt": "2020-06-10T18:58:21.378752Z",
//       "updatedAt": "2020-06-10T18:58:21.378752Z",
//       "currency": "BAT",
//       "quantity": 4,
//       "price": "0.25",
//       "subtotal": "1",
//       "location": "brave.com",
//       "description": ""
//     }
//   ]
// }

namespace ledger {

class PostOrderEndpoint : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "payments-post-order-endpoint";

  URLRequest MapRequest(const std::map<std::string, int>& items);

  absl::optional<PaymentOrder> MapResponse(const URLResponse& response);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_ORDER_ENDPOINT_H_

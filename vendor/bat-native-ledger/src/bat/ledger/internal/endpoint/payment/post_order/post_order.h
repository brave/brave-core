/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PAYMENT_POST_ORDER_POST_ORDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PAYMENT_POST_ORDER_POST_ORDER_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

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
class LedgerImpl;

namespace endpoint {
namespace payment {

using PostOrderCallback = std::function<void(
    const type::Result result,
    type::SKUOrderPtr order)>;

class PostOrder {
 public:
  explicit PostOrder(LedgerImpl* ledger);
  ~PostOrder();

  void Request(
      std::vector<type::SKUOrderItemPtr> items,
      PostOrderCallback callback);

 private:
  std::string GetUrl();

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(
      const std::string& body,
      std::vector<mojom::SKUOrderItemPtr> order_items,
      type::SKUOrder* order);

  void OnRequest(
      const type::UrlResponse& response,
      std::shared_ptr<std::vector<mojom::SKUOrderItemPtr>> items,
      PostOrderCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PAYMENT_POST_ORDER_POST_ORDER_H_

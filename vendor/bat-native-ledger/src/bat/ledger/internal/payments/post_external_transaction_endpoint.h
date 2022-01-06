/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_EXTERNAL_TRANSACTION_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_EXTERNAL_TRANSACTION_ENDPOINT_H_

#include <string>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/url_fetcher.h"
#include "bat/ledger/internal/external_wallet/external_wallet_data.h"

// POST /v1/orders/{order_id}/transactions/{provider}
//
// Request body:
// {
//   "externalTransactionId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "kind": "uphold"
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

class PostExternalTransactionEndpoint : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] =
      "payments-post-external-transaction-endpoint";

  URLRequest MapRequest(const std::string& order_id,
                        const std::string& transaction_id,
                        ExternalWalletProvider provider);

  bool MapResponse(const URLResponse& response);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_EXTERNAL_TRANSACTION_ENDPOINT_H_

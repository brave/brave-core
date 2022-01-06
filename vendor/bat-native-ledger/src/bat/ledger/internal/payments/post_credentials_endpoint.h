/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_CREDENTIALS_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_CREDENTIALS_ENDPOINT_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/url_fetcher.h"
#include "bat/ledger/internal/payments/payment_data.h"

// POST /v1/orders/{order_id}/credentials
//
// Request body:
// {
//   "itemId": "ff50981d-47de-4210-848d-995e186901a1",
//   "type": "single-use",
//   "blindedCreds": [
//     "wqto9FnferrKUM0lcp2B0lecMQwArvUq3hWGCYlXiQo=",
//     "ZiSXpF61aZ/tL2MxkKzI5Vnw2aLJE2ln2FMHAtKc9Co="
//   ]
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_CONFLICT (409)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {Empty}

namespace ledger {

class PostCredentialsEndpoint : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "payments-post-credentials-endpoint";

  URLRequest MapRequest(const std::string& order_id,
                        const std::string& item_id,
                        PaymentCredentialType type,
                        const std::vector<std::string>& blinded_tokens);

  bool MapResponse(const URLResponse& response);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_CREDENTIALS_ENDPOINT_H_

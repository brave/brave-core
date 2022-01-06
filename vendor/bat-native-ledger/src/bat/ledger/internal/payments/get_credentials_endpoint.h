/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_GET_CREDENTIALS_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_GET_CREDENTIALS_ENDPOINT_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/url_fetcher.h"
#include "bat/ledger/internal/payments/payment_data.h"

// GET /v1/orders/{order_id}/credentials/{order_item_id}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_ACCEPTED (202)
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body (success):
// {
//   "id": "9c9aed7f-b349-452e-80a8-95faf2b1600d",
//   "orderId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "issuerId": "138bf9ca-69fe-4540-9ac4-bc65baddc4a0",
//   "signedCreds": [
//     "ijSZoLLG+EnRN916RUQcjiV6c4Wb6ItbnxXBFhz81EQ=",
//     "dj6glCJ2roHYcTFcXF21IrKx1uT/ptM7SJEdiEE1fG8=",
//     "nCF9a4KuASICVC0zrx2wGnllgIUxBMnylpu5SA+oBjI="
//   ],
//   "batchProof": "zx0cdJhaB/OdYcUtnyXdi+lsoniN2vRTZ1w0U4D7Mgeu1I7RwB+tYKNgFU",
//   "publicKey": "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I="
// }
//
// Response body (error):
// {
//   "message": "Claim has been accepted but is not ready",
//   "code": 202,
//   "data": {}
// }

namespace ledger {

class GetCredentialsEndpoint : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "payments-get-credentials-endpoint";

  URLRequest MapRequest(const std::string& order_id,
                        const std::string& item_id);

  absl::optional<PaymentCredentials> MapResponse(const URLResponse& response);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_GET_CREDENTIALS_ENDPOINT_H_

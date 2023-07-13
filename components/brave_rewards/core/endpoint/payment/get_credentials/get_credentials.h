/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_GET_CREDENTIALS_GET_CREDENTIALS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_GET_CREDENTIALS_GET_CREDENTIALS_H_

#include <memory>
#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

// GET /v1/promotions/{promotion_id}/claims/{claim_id}
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

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace payment {

using GetCredentialsCallback =
    base::OnceCallback<void(mojom::Result, mojom::CredsBatchPtr)>;

class GetCredentials {
 public:
  explicit GetCredentials(RewardsEngineImpl& engine);
  ~GetCredentials();

  void Request(const std::string& order_id,
               const std::string& item_id,
               GetCredentialsCallback callback);

 private:
  std::string GetUrl(const std::string& order_id, const std::string& item_id);

  mojom::Result CheckStatusCode(const int status_code);

  mojom::Result ParseBody(const std::string& body, mojom::CredsBatch* batch);

  void OnRequest(GetCredentialsCallback callback,
                 mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_GET_CREDENTIALS_GET_CREDENTIALS_H_

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_POST_TRANSACTION_GEMINI_POST_TRANSACTION_SKU_GEMINI_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_POST_TRANSACTION_GEMINI_POST_TRANSACTION_SKU_GEMINI_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"

// POST /v1/orders/{order_id}/transactions/gemini
//
// Request body:
// {
//   "externalTransactionId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "kind": "gemini"
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
//   "kind": "gemini",
//   "amount": "1"
// }

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoint {
namespace payment {

using PostTransactionGeminiCallback =
    base::OnceCallback<void(const mojom::Result result)>;

class PostTransactionGemini {
 public:
  explicit PostTransactionGemini(RewardsEngine& engine);
  ~PostTransactionGemini();

  void Request(const mojom::SKUTransaction& transaction,
               PostTransactionGeminiCallback callback);

 private:
  std::string GetUrl(const std::string& order_id);

  std::string GeneratePayload(const mojom::SKUTransaction& transaction);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(PostTransactionGeminiCallback callback,
                 mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngine> engine_;
};

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_POST_TRANSACTION_GEMINI_POST_TRANSACTION_SKU_GEMINI_H_

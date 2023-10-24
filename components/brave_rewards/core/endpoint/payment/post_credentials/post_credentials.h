/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_POST_CREDENTIALS_POST_CREDENTIALS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_POST_CREDENTIALS_POST_CREDENTIALS_H_

#include <memory>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

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

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace payment {

using PostCredentialsCallback = base::OnceCallback<void(mojom::Result)>;

class PostCredentials {
 public:
  explicit PostCredentials(RewardsEngineImpl& engine);
  ~PostCredentials();

  void Request(const std::string& order_id,
               const std::string& item_id,
               const std::string& type,
               base::Value::List&& blinded_creds,
               PostCredentialsCallback callback);

 private:
  std::string GetUrl(const std::string& order_id);

  std::string GeneratePayload(const std::string& item_id,
                              const std::string& type,
                              base::Value::List&& blinded_creds);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(PostCredentialsCallback callback,
                 mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_POST_CREDENTIALS_POST_CREDENTIALS_H_

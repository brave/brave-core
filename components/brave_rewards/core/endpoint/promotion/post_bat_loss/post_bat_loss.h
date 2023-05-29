/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_BAT_LOSS_POST_BAT_LOSS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_BAT_LOSS_POST_BAT_LOSS_H_

#include <string>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"

// POST /v1/wallets/{payment_id}/events/batloss/{version}
//
// Request body:
// {
//   "amount": 20.5
// }
//
// Success code:
// HTTP_OK (200)
//
// Error Codes:
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {Empty}

namespace brave_rewards::internal::endpoint::promotion {

using PostBatLossCallback = std::function<void(const mojom::Result result)>;

class PostBatLoss {
 public:
  void Request(const double amount,
               const int32_t version,
               PostBatLossCallback callback);

 private:
  std::string GetUrl(const int32_t version);

  std::string GeneratePayload(const double amount);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(mojom::UrlResponsePtr response, PostBatLossCallback callback);
};

}  // namespace brave_rewards::internal::endpoint::promotion

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_POST_BAT_LOSS_POST_BAT_LOSS_H_

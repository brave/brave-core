/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_BITFLYER_GET_BALANCE_GET_BALANCE_BITFLYER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_BITFLYER_GET_BALANCE_GET_BALANCE_BITFLYER_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

// GET https://bitflyer.com/api/link/v1/account/inventory
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "account_hash": "0123456789",
//   "inventory": [
//     {
//       "currency_code": "JPY",
//       "amount": 1024078,
//       "available": 508000
//     },
//     {
//       "currency_code": "BTC",
//       "amount": 10.24,
//       "available": 4.12
//     },
//     {
//       "currency_code": "ETH",
//       "amount": 10.24,
//       "available": 4.12
//     }
//   ]
// }

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace bitflyer {

using GetBalanceCallback = base::OnceCallback<void(const mojom::Result result,
                                                   const double available)>;

class GetBalance {
 public:
  explicit GetBalance(RewardsEngineImpl& engine);
  ~GetBalance();

  void Request(const std::string& token, GetBalanceCallback callback);

 private:
  std::string GetUrl();

  mojom::Result CheckStatusCode(const int status_code);

  mojom::Result ParseBody(const std::string& body, double* available);

  void OnRequest(GetBalanceCallback callback, mojom::UrlResponsePtr response);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_BITFLYER_GET_BALANCE_GET_BALANCE_BITFLYER_H_

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_GET_WALLET_GET_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_GET_WALLET_GET_WALLET_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

// GET /v3/wallet/{payment_id}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
//
// Response body:
// {
//     "paymentId": "368d87a3-7749-4ebb-9f3a-2882c99078c7",
//     "depositAccountProvider": {
//         "name": "uphold",
//         "id": "",
//         "linkingId": "4668ba96-7129-5e85-abdc-0c144ab78834"
//     },
//     "walletProvider": {
//         "id": "",
//         "name": "brave"
//     },
//     "altcurrency": "BAT",
//     "publicKey":
//     "ae55f61fa5b2870c0ee3633004c6d7a40adb5694c73d05510d8179cec8a3403a"
// }

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {
namespace promotion {

using GetWalletCallback = std::function<
    void(mojom::Result result, const std::string& custodian, bool linked)>;

class GetWallet {
 public:
  explicit GetWallet(RewardsEngineImpl& engine);
  ~GetWallet();

  void Request(GetWalletCallback callback) const;

 private:
  std::string GetUrl() const;

  void OnRequest(mojom::UrlResponsePtr response,
                 GetWalletCallback callback) const;

  mojom::Result CheckStatusCode(int status_code) const;

  mojom::Result ParseBody(const std::string& body,
                          std::string* custodian,
                          bool* linked) const;

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_GET_WALLET_GET_WALLET_H_

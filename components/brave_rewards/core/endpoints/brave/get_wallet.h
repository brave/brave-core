/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_WALLET_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/endpoints/response_handler.h"
#include "brave/components/brave_rewards/core/endpoints/result_for.h"

// GET /v4/wallets/{payment_id}
//
// clang-format off
// Response body:
// {
//   "altcurrency": "BAT",
//   "depositAccountProvider": {
//     "id": "2d7519f4-cb7b-41b7-9f33-9d716f2e7915",
//     "linkingId": "2698ba94-7129-5a85-abcd-0c166ab75189",
//     "name": "uphold"
//   },
//   "paymentId": "f6d73e13-abcd-56fc-ab96-f4c3efcc7185",
//   "publicKey": "33a7887a935977de43a1495281142b872e2b0e94bf25a18aed7272b397759184",
//   "walletProvider": {
//     "id": "",
//     "name": "brave"
//   },
//   "selfCustodyAvailable": {
//     "solana": true
//   }
// }
// clang-format on

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoints {

struct GetWalletValue {
  GetWalletValue();
  ~GetWalletValue();

  GetWalletValue(const GetWalletValue&) = delete;
  GetWalletValue& operator=(const GetWalletValue&) = delete;

  GetWalletValue(GetWalletValue&&);
  GetWalletValue& operator=(GetWalletValue&&);

  std::string wallet_provider;
  std::string provider_id;
  bool linked = false;
  base::Value::Dict self_custody_available;
};

class GetWallet;

template <>
struct ResultFor<GetWallet> {
  using Value = GetWalletValue;
  using Error = mojom::GetWalletError;
};

class GetWallet final : public RequestBuilder,
                        public ResponseHandler<GetWallet> {
 public:
  static Result ProcessResponse(RewardsEngine& engine,
                                const mojom::UrlResponse& response);

  explicit GetWallet(RewardsEngine& engine);
  ~GetWallet() override;

 private:
  std::optional<std::string> Url() const override;
  mojom::UrlMethod Method() const override;
  std::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
};

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_WALLET_H_

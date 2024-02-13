/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ZEBPAY_ZEBPAY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ZEBPAY_ZEBPAY_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/zebpay/get_balance_zebpay.h"
#include "brave/components/brave_rewards/core/wallet_provider/wallet_provider.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace zebpay {

class ZebPay final : public wallet_provider::WalletProvider {
 public:
  explicit ZebPay(RewardsEngineImpl& engine);

  const char* WalletType() const override;

  void AssignWalletLinks(mojom::ExternalWallet& external_wallet) override;

  void FetchBalance(
      base::OnceCallback<void(mojom::Result, double)> callback) override;

  std::string GetFeeAddress() const override;

 private:
  void OnFetchBalance(base::OnceCallback<void(mojom::Result, double)> callback,
                      endpoints::GetBalanceZebPay::Result&& result);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace zebpay
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ZEBPAY_ZEBPAY_H_

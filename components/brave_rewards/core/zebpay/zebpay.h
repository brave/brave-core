/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ZEBPAY_ZEBPAY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ZEBPAY_ZEBPAY_H_

#include <set>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/endpoints/zebpay/get_balance/get_balance_zebpay.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/wallet_provider/zebpay/connect_zebpay_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/zebpay/get_zebpay_wallet.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace zebpay {

using FetchBalanceCallback = base::OnceCallback<void(mojom::Result, double)>;

class ZebPay {
 public:
  explicit ZebPay(RewardsEngineImpl& engine);

  ~ZebPay();

  void FetchBalance(FetchBalanceCallback callback);

  void ConnectWallet(const base::flat_map<std::string, std::string>& args,
                     ConnectExternalWalletCallback callback);

  void GetWallet(GetExternalWalletCallback callback);

  mojom::ExternalWalletPtr GetWallet();

  mojom::ExternalWalletPtr GetWalletIf(
      const std::set<mojom::WalletStatus>& statuses);

  [[nodiscard]] bool SetWallet(mojom::ExternalWalletPtr wallet);

  [[nodiscard]] bool LogOutWallet();

 private:
  void OnFetchBalance(FetchBalanceCallback callback,
                      endpoints::GetBalanceZebPay::Result&& result);

  const raw_ref<RewardsEngineImpl> engine_;
  ConnectZebPayWallet connect_wallet_;
  GetZebPayWallet get_wallet_;
};

}  // namespace zebpay
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ZEBPAY_ZEBPAY_H_

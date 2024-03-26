/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_SOLANA_SOLANA_WALLET_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_SOLANA_SOLANA_WALLET_PROVIDER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/endpoints/brave/post_challenges.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"
#include "brave/components/brave_rewards/core/wallet_provider/wallet_provider.h"

namespace brave_rewards::internal {

class SolanaWalletProvider : public RewardsEngineHelper,
                             public WithHelperKey<SolanaWalletProvider>,
                             public wallet_provider::WalletProvider {
 public:
  explicit SolanaWalletProvider(RewardsEngine& engine);
  ~SolanaWalletProvider() override;

  const char* WalletType() const override;

  void AssignWalletLinks(mojom::ExternalWallet& external_wallet) override;

  void FetchBalance(
      base::OnceCallback<void(mojom::Result, double)> callback) override;

  void BeginLogin(BeginExternalWalletLoginCallback callback) override;

  std::string GetFeeAddress() const override;

  void OnWalletLinked(const std::string& address) override;

  void PollWalletStatus();

 private:
  void OnPostChallengesResponse(BeginExternalWalletLoginCallback callback,
                                endpoints::PostChallenges::Result result);

  void OnAccountBalanceFetched(
      base::OnceCallback<void(mojom::Result, double)> callback,
      mojom::SolanaAccountBalancePtr balance);

  void OnPollingTimeout();

  base::RepeatingTimer polling_timer_;
  base::OneShotTimer polling_timeout_;
  base::WeakPtrFactory<SolanaWalletProvider> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_SOLANA_SOLANA_WALLET_PROVIDER_H_

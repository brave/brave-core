/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_WALLET_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_WALLET_PROVIDER_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/transfer.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace wallet_provider {

class WalletProvider {
 public:
  explicit WalletProvider(RewardsEngine& engine);
  virtual ~WalletProvider();

  virtual const char* WalletType() const = 0;

  virtual void FetchBalance(
      base::OnceCallback<void(mojom::Result, double)> callback) = 0;

  virtual std::string GetFeeAddress() const = 0;

  virtual base::TimeDelta GetDelay() const;

  virtual void AssignWalletLinks(mojom::ExternalWallet& external_wallet) = 0;

  virtual void OnWalletLinked(const std::string& address);

  void Initialize();

  void StartContribution(const std::string& contribution_id,
                         mojom::ServerPublisherInfoPtr info,
                         double amount,
                         ResultCallback callback);

  virtual void BeginLogin(BeginExternalWalletLoginCallback callback);

  void ConnectWallet(const base::flat_map<std::string, std::string>& args,
                     ConnectExternalWalletCallback callback);

  mojom::ExternalWalletPtr GetWallet();

  mojom::ExternalWalletPtr GetWalletIf(
      const std::set<mojom::WalletStatus>& statuses);

  [[nodiscard]] bool SetWallet(mojom::ExternalWalletPtr wallet);

  [[nodiscard]] bool LogOutWallet(const std::string& notification = "");

 protected:
  void OnFetchBalance(base::OnceCallback<void(mojom::Result, double)> callback,
                      mojom::Result result,
                      double available);

 private:
  void ContributionCompleted(ResultCallback callback,
                             const std::string& contribution_id,
                             double fee,
                             const std::string& publisher_key,
                             mojom::Result result);

  void SaveTransferFee(const std::string& contribution_id, double amount);

  void StartTransferFeeTimer(const std::string& fee_id, int attempts);

  void OnTransferFeeCompleted(const std::string& contribution_id,
                              int attempts,
                              mojom::Result result);

  void TransferFee(const std::string& contribution_id,
                   double amount,
                   int attempts);

  void OnTransferFeeTimerElapsed(const std::string& id, int attempts);

  void RemoveTransferFee(const std::string& contribution_id);

 protected:
  const raw_ref<RewardsEngine> engine_;
  std::unique_ptr<ConnectExternalWallet> connect_wallet_;
  std::unique_ptr<Transfer> transfer_;

 private:
  std::map<std::string, base::OneShotTimer> transfer_fee_timers_;
};

}  // namespace wallet_provider
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_WALLET_PROVIDER_H_

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_H_

#include <stdint.h>

#include <map>
#include <set>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/get_capabilities/get_capabilities.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/get_me/get_me.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/uphold_server.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/uphold/uphold_card.h"
#include "brave/components/brave_rewards/core/uphold/uphold_user.h"
#include "brave/components/brave_rewards/core/wallet_provider/uphold/connect_uphold_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/uphold/get_uphold_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/uphold/uphold_transfer.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace uphold {

using FetchBalanceCallback = base::OnceCallback<void(mojom::Result, double)>;
using endpoint::uphold::GetCapabilitiesCallback;
using endpoint::uphold::GetMeCallback;

class Uphold {
 public:
  explicit Uphold(RewardsEngineImpl& engine);

  ~Uphold();

  void Initialize();

  void StartContribution(const std::string& contribution_id,
                         mojom::ServerPublisherInfoPtr info,
                         double amount,
                         LegacyResultCallback callback);

  void FetchBalance(FetchBalanceCallback);

  void TransferFunds(double amount,
                     const std::string& address,
                     const std::string& contribution_id,
                     LegacyResultCallback);

  void ConnectWallet(const base::flat_map<std::string, std::string>& args,
                     ConnectExternalWalletCallback);

  void GetWallet(GetExternalWalletCallback);

  void CreateCard(const std::string& access_token, CreateCardCallback);

  void GetUser(const std::string& access_token, GetMeCallback);

  void GetCapabilities(const std::string& access_token,
                       GetCapabilitiesCallback);

  mojom::ExternalWalletPtr GetWallet();

  mojom::ExternalWalletPtr GetWalletIf(const std::set<mojom::WalletStatus>&);

  [[nodiscard]] bool SetWallet(mojom::ExternalWalletPtr);

  [[nodiscard]] mojom::ExternalWalletPtr TransitionWallet(
      mojom::ExternalWalletPtr,
      mojom::WalletStatus to);

  [[nodiscard]] bool LogOutWallet(const std::string& notification = "");

 private:
  void ContributionCompleted(LegacyResultCallback,
                             const std::string& contribution_id,
                             double fee,
                             const std::string& publisher_key,
                             mojom::Result);

  void OnFetchBalance(FetchBalanceCallback, mojom::Result, double available);

  void SaveTransferFee(const std::string& contribution_id, double amount);

  void StartTransferFeeTimer(const std::string& fee_id, int attempts);

  void OnTransferFeeCompleted(const std::string& contribution_id,
                              int attempts,
                              mojom::Result);

  void TransferFee(const std::string& contribution_id,
                   double amount,
                   int attempts);

  void OnTransferFeeTimerElapsed(const std::string& id, int attempts);

  void RemoveTransferFee(const std::string& contribution_id);

  const raw_ref<RewardsEngineImpl> engine_;
  UpholdCard card_;
  ConnectUpholdWallet connect_wallet_;
  GetUpholdWallet get_wallet_;
  UpholdTransfer transfer_;
  endpoint::UpholdServer uphold_server_;
  std::map<std::string, base::OneShotTimer> transfer_fee_timers_;
};

}  // namespace uphold
}  // namespace brave_rewards::internal
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_H_

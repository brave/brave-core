/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_GEMINI_GEMINI_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_GEMINI_GEMINI_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/get_external_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/transfer.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace endpoint {
class GeminiServer;
}

namespace gemini {

using FetchBalanceCallback = base::OnceCallback<void(mojom::Result, double)>;

class Gemini {
 public:
  explicit Gemini(LedgerImpl*);

  ~Gemini();

  void Initialize();

  void StartContribution(const std::string& contribution_id,
                         mojom::ServerPublisherInfoPtr info,
                         double amount,
                         LegacyResultCallback callback);

  void FetchBalance(FetchBalanceCallback callback);

  void TransferFunds(double amount,
                     const std::string& address,
                     const std::string& contribution_id,
                     LegacyResultCallback);

  void ConnectWallet(const base::flat_map<std::string, std::string>& args,
                     ConnectExternalWalletCallback);

  void GetWallet(GetExternalWalletCallback);

  mojom::ExternalWalletPtr GetWallet();

  mojom::ExternalWalletPtr GetWalletIf(const std::set<mojom::WalletStatus>&);

  [[nodiscard]] bool SetWallet(mojom::ExternalWalletPtr);

  [[nodiscard]] bool LogOutWallet();

 private:
  void ContributionCompleted(LegacyResultCallback,
                             const std::string& contribution_id,
                             double fee,
                             const std::string& publisher_key,
                             mojom::Result);

  void OnFetchBalance(FetchBalanceCallback, mojom::Result, double available);

  void SaveTransferFee(const std::string& contribution_id, double amount);

  void StartTransferFeeTimer(const std::string& fee_id, const int attempts);

  void OnTransferFeeCompleted(const std::string& contribution_id,
                              int attempts,
                              mojom::Result);

  void TransferFee(const std::string& contribution_id,
                   double amount,
                   int attempts);

  void OnTransferFeeTimerElapsed(const std::string& id, const int attempts);

  void RemoveTransferFee(const std::string& contribution_id);

  std::unique_ptr<wallet_provider::ConnectExternalWallet> connect_wallet_;
  std::unique_ptr<wallet_provider::GetExternalWallet> get_wallet_;
  std::unique_ptr<wallet_provider::Transfer> transfer_;
  std::unique_ptr<endpoint::GeminiServer> gemini_server_;
  LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, base::OneShotTimer> transfer_fee_timers_;
};

}  // namespace gemini
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_GEMINI_GEMINI_H_

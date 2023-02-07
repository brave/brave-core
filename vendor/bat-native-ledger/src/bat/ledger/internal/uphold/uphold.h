/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"
#include "base/timer/timer.h"
#include "bat/ledger/internal/endpoint/uphold/get_capabilities/get_capabilities.h"
#include "bat/ledger/internal/endpoint/uphold/get_me/get_me.h"
#include "bat/ledger/internal/uphold/uphold_user.h"
#include "bat/ledger/internal/wallet_provider/connect_external_wallet.h"
#include "bat/ledger/internal/wallet_provider/get_external_wallet.h"
#include "bat/ledger/internal/wallet_provider/transfer.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class UpholdServer;
}

namespace uphold {

class UpholdCard;

using FetchBalanceCallback = base::OnceCallback<void(mojom::Result, double)>;
using CreateCardCallback =
    base::OnceCallback<void(mojom::Result, std::string&& id)>;
using endpoint::uphold::GetCapabilitiesCallback;
using endpoint::uphold::GetMeCallback;

class Uphold {
 public:
  explicit Uphold(LedgerImpl*);

  ~Uphold();

  void Initialize();

  void StartContribution(const std::string& contribution_id,
                         mojom::ServerPublisherInfoPtr info,
                         double amount,
                         ledger::LegacyResultCallback callback);

  void FetchBalance(FetchBalanceCallback);

  void TransferFunds(double amount,
                     const std::string& address,
                     const std::string& contribution_id,
                     client::LegacyResultCallback);

  void ConnectWallet(const base::flat_map<std::string, std::string>& args,
                     ledger::ConnectExternalWalletCallback);

  void GetWallet(ledger::GetExternalWalletCallback);

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
  void ContributionCompleted(ledger::LegacyResultCallback,
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

  std::unique_ptr<UpholdCard> card_;
  std::unique_ptr<wallet_provider::ConnectExternalWallet> connect_wallet_;
  std::unique_ptr<wallet_provider::GetExternalWallet> get_wallet_;
  std::unique_ptr<wallet_provider::Transfer> transfer_;
  std::unique_ptr<endpoint::UpholdServer> uphold_server_;
  LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, base::OneShotTimer> transfer_fee_timers_;
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_H_

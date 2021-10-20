/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/timer/timer.h"
#include "bat/ledger/internal/uphold/uphold_user.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class UpholdServer;
}

namespace uphold {

struct Transaction {
  std::string address;
  double amount;
  std::string message;
};

class UpholdTransfer;
class UpholdCard;
class UpholdAuthorization;
class UpholdWallet;

using FetchBalanceCallback = std::function<void(type::Result, double)>;
using CreateCardCallback =
    std::function<void(type::Result, const std::string&)>;

class Uphold {
 public:
  explicit Uphold(LedgerImpl* ledger);

  ~Uphold();

  void Initialize();

  void StartContribution(const std::string& contribution_id,
                         type::ServerPublisherInfoPtr info,
                         const double amount,
                         ledger::ResultCallback callback);

  void FetchBalance(FetchBalanceCallback callback);

  void TransferFunds(const double amount,
                     const std::string& address,
                     client::TransactionCallback callback);

  void WalletAuthorization(
      const base::flat_map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback);

  void GenerateWallet(ledger::ResultCallback callback);

  void CreateCard(CreateCardCallback callback);

  void DisconnectWallet(const absl::optional<std::string>& notification);

  void GetUser(GetUserCallback callback);

  type::ExternalWalletPtr GetWallet();

  bool SetWallet(type::ExternalWalletPtr wallet);

 private:
  void ContributionCompleted(const type::Result result,
                             const std::string& transaction_id,
                             const std::string& contribution_id,
                             const double fee,
                             const std::string& publisher_key,
                             ledger::ResultCallback callback);

  void OnFetchBalance(const type::Result result,
                      const double available,
                      FetchBalanceCallback callback);

  void SaveTransferFee(const std::string& contribution_id, const double amount);

  void StartTransferFeeTimer(const std::string& fee_id, int attempts);

  void OnTransferFeeCompleted(const type::Result result,
                              const std::string& transaction_id,
                              const std::string& contribution_id,
                              int attempts);

  void TransferFee(const std::string& contribution_id,
                   const double amount,
                   int attempts);

  void OnTransferFeeTimerElapsed(const std::string& id, int attempts);

  void RemoveTransferFee(const std::string& contribution_id);

  std::unique_ptr<UpholdTransfer> transfer_;
  std::unique_ptr<UpholdCard> card_;
  std::unique_ptr<UpholdUser> user_;
  std::unique_ptr<UpholdAuthorization> authorization_;
  std::unique_ptr<UpholdWallet> wallet_;
  std::unique_ptr<endpoint::UpholdServer> uphold_server_;
  LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, base::OneShotTimer> transfer_fee_timers_;
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_H_

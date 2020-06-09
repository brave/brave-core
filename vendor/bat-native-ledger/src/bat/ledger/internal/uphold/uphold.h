/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_H_

#include <stdint.h>

#include <string>
#include <map>
#include <memory>

#include "base/timer/timer.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold_user.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

struct Transaction {
  std::string address;
  double amount;
  std::string message;
};

class UpholdTransfer;
class UpholdCard;
class UpholdAuthorization;
class UpholdWallet;

using FetchBalanceCallback = std::function<void(ledger::Result, double)>;
using CreateCardCallback =
    std::function<void(ledger::Result, const std::string&)>;

class Uphold {
 public:
  explicit Uphold(bat_ledger::LedgerImpl* ledger);

  ~Uphold();

  void Initialize();

  void StartContribution(
      const std::string& contribution_id,
      ledger::ServerPublisherInfoPtr info,
      const double amount,
      ledger::ResultCallback callback);

  void FetchBalance(FetchBalanceCallback callback);

  void TransferFunds(
      const double amount,
      const std::string& address,
      ledger::TransactionCallback callback);

  void WalletAuthorization(
      const std::map<std::string, std::string>& args,
      ledger::ExternalWalletAuthorizationCallback callback);

  void GenerateExternalWallet(ledger::ResultCallback callback);

  void CreateCard(CreateCardCallback callback);

  void DisconnectWallet();

  void GetUser(GetUserCallback callback);

  void CreateAnonAddressIfNecessary(ledger::ResultCallback callback);

 private:
  void ContributionCompleted(
      const ledger::Result result,
      const std::string& transaction_id,
      const std::string& contribution_id,
      const double fee,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  void OnFetchBalance(
      const ledger::UrlResponse& response,
      FetchBalanceCallback callback);

  void SaveTransferFee(ledger::TransferFeePtr transfer_fee);

  void StartTransferFeeTimer(const std::string& fee_id);

  void OnTransferFeeCompleted(
      const ledger::Result result,
      const std::string& transaction_id,
      const ledger::TransferFee& transfer_fee);

  void TransferFee(const ledger::TransferFee& transfer_fee);

  void OnTransferFeeTimerElapsed(const std::string& id);

  std::unique_ptr<UpholdTransfer> transfer_;
  std::unique_ptr<UpholdCard> card_;
  std::unique_ptr<UpholdUser> user_;
  std::unique_ptr<UpholdAuthorization> authorization_;
  std::unique_ptr<UpholdWallet> wallet_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::map<std::string, base::OneShotTimer> transfer_fee_timers_;
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_H_

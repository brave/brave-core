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
using CreateAnonAddressCallback =
    std::function<void(ledger::Result, const std::string&)>;

class Uphold {
 public:
  explicit Uphold(bat_ledger::LedgerImpl* ledger);

  ~Uphold();

  void Initialize();

  void StartContribution(
      const std::string& contribution_id,
      ledger::ServerPublisherInfoPtr info,
      double amount,
      ledger::ExternalWalletPtr wallet,
      ledger::ResultCallback callback);

  void FetchBalance(std::map<std::string, ledger::ExternalWalletPtr> wallets,
                    FetchBalanceCallback callback);

  void TransferFunds(
      const double amount,
      const std::string& address,
      ledger::ExternalWalletPtr wallet,
      ledger::TransactionCallback callback);

  void WalletAuthorization(
    const std::map<std::string, std::string>& args,
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletAuthorizationCallback callback);

  void TransferAnonToExternalWallet(
      ledger::ExternalWalletPtr wallet,
      ledger::ExternalWalletCallback callback);

  void GenerateExternalWallet(
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletCallback callback);

  void CreateCard(
      ledger::ExternalWalletPtr wallet,
      CreateCardCallback callback);

  void DisconnectWallet();

  void GetUser(
    ledger::ExternalWalletPtr wallet,
    GetUserCallback callback);

  void CreateAnonAddressIfNecessary(
      ledger::ExternalWalletPtr wallet,
      CreateAnonAddressCallback callback);

  void OnTimer(uint32_t timer_id);

 private:
  void ContributionCompleted(
      const ledger::Result result,
      const std::string& transaction_id,
      const std::string& contribution_id,
      const double fee,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  void OnFetchBalance(
    FetchBalanceCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void OnTransferAnonToExternalWalletCallback(
    ledger::ExternalWalletCallback callback,
    const ledger::ExternalWallet& wallet,
    ledger::Result result);

  void OnDisconectWallet(
    ledger::Result result,
    ledger::ExternalWalletPtr wallet);

  void SaveTransferFee(ledger::TransferFeePtr transfer_fee);

  void OnTransferFeeCompleted(
    const ledger::Result result,
    const std::string& transaction_id,
    const ledger::TransferFee& transfer_fee);

  void TransferFee(
    const ledger::Result result,
    ledger::ExternalWalletPtr wallet,
    const ledger::TransferFee& transfer_fee);

  void TransferFeeOnTimer(const uint32_t timer_id);

  void SetTimer(uint32_t* timer_id, uint64_t start_timer_in = 0);

  std::unique_ptr<UpholdTransfer> transfer_;
  std::unique_ptr<UpholdCard> card_;
  std::unique_ptr<UpholdUser> user_;
  std::unique_ptr<UpholdAuthorization> authorization_;
  std::unique_ptr<UpholdWallet> wallet_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_H_

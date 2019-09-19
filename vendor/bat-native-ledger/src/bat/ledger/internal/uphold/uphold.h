/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_H_

#include <string>
#include <map>
#include <memory>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold_user.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

class UpholdTransfer;
class UpholdCard;
class UpholdAuthorization;
class UpholdWallet;

using TransactionCallback = std::function<void(ledger::Result, bool created)>;
using FetchBalanceCallback = std::function<void(ledger::Result, double)>;
using CreateCardCallback =
    std::function<void(ledger::Result, const std::string&)>;
using CreateAnonAddressCallback =
    std::function<void(ledger::Result, const std::string&)>;

class Uphold {
 public:
  explicit Uphold(bat_ledger::LedgerImpl* ledger);

  ~Uphold();

  void StartContribution(
      const std::string& viewing_id,
      const std::string& address,
      double amount,
      ledger::ExternalWalletPtr wallet);

  void FetchBalance(std::map<std::string, ledger::ExternalWalletPtr> wallets,
                    FetchBalanceCallback callback);

  void TransferFunds(double amount,
                     const std::string& address,
                     ledger::ExternalWalletPtr wallet,
                     TransactionCallback callback);

  void WalletAuthorization(
    const std::map<std::string, std::string>& args,
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletAuthorizationCallback callback);

  void TransferAnonToExternalWallet(
      ledger::ExternalWalletPtr wallet,
      const bool allow_zero_balance,
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

 private:
  void ContributionCompleted(
      const ledger::Result result,
      const bool created,
      const std::string& viewing_id,
      const double fee = 0,
      const ledger::ExternalWallet& wallet = {});

  void OnFeeCompleted(ledger::Result result,
                    bool created,
                    const std::string &viewing_id);

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

  std::unique_ptr<UpholdTransfer> transfer_;
  std::unique_ptr<UpholdCard> card_;
  std::unique_ptr<UpholdUser> user_;
  std::unique_ptr<UpholdAuthorization> authorization_;
  std::unique_ptr<UpholdWallet> wallet_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_H_

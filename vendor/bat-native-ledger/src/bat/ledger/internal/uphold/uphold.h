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

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {
class UpholdTransfer;
}

namespace braveledger_uphold {

using TransactionCallback = std::function<void(ledger::Result, bool created)>;
using FetchBalanceCallback = std::function<void(ledger::Result, double)>;

class Uphold {
 public:
  explicit Uphold(bat_ledger::LedgerImpl* ledger);

  ~Uphold();

  void StartContribution(const std::string &viewing_id,
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

  void GenerateExternalWallet(
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletCallback callback);

 private:
  void ContributionCompleted(ledger::Result result,
                             bool created,
                             const std::string &viewing_id);

  void OnFeeCompleted(ledger::Result result,
                    bool created,
                    const std::string &viewing_id);

  void OnFetchBalance(
    FetchBalanceCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void OnWalletAuthorization(
    ledger::ExternalWalletAuthorizationCallback callback,
    ledger::ExternalWallet wallet,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  std::unique_ptr<UpholdTransfer> transfer_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_H_

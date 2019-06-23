/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_H_

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {
class UpholdTransfer;
}

namespace braveledger_uphold {

const char kUrlStaging[] = "https://api-sandbox.uphold.com";
const char kUrlProduction[] = "https://api.uphold.com";
const char kClientIdStaging[] = "4c2b665ca060d912fec5c735c734859a06118cc8";
const char kClientIdProduction[] = "";
const char kClientSecretStaging[] = "67bf87da096748c5bc1e195cfbdd59db006618a0";
const char kClientSecretProduction[] = "";
const char kFeeAddressStaging[] = "7ab330ae-b7a6-4845-8f07-baab68848e4e";
const char kFeeAddressProduction[] = "";

using TransactionCallback = std::function<void(ledger::Result, bool created)>;
using FetchBalanceCallback = std::function<void(ledger::Result, double)>;

class Uphold {
 public:
  explicit Uphold(bat_ledger::LedgerImpl* ledger);

  ~Uphold();
  static std::vector<std::string> RequestAuthorization(
      const std::string& token);

  static ledger::ExternalWalletPtr GetWallet(
      std::map<std::string, ledger::ExternalWalletPtr> wallets);

  static std::string GetAPIUrl(const std::string& path);

  void StartContribution(const std::string &viewing_id,
                         ledger::ExternalWalletPtr wallet);

  void FetchBalance(std::map<std::string, ledger::ExternalWalletPtr> wallets,
                    FetchBalanceCallback callback);

  void TransferFunds(double amount,
                     const std::string& address,
                     ledger::ExternalWalletPtr wallet,
                     TransactionCallback callback);

 private:
  static std::string GetFeeAddress();

  static std::string ConvertToProbi(const std::string& amount);

  void ContributionCompleted(ledger::Result result,
                             bool created,
                             const std::string &viewing_id);

  void FeeCompleted(ledger::Result result,
                    bool created,
                    const std::string &viewing_id);

  void OnFetchBalance(
    FetchBalanceCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  std::unique_ptr<UpholdTransfer> transfer_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_H_

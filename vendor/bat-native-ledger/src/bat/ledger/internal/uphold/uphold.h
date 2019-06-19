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
class UpholdContribution;
}

namespace braveledger_uphold {

const char kUrlStaging[] = "https://api-sandbox.uphold.com";
const char kUrlProduction[] = "https://api.uphold.com";
const char kClientIdStaging[] = "4c2b665ca060d912fec5c735c734859a06118cc8";
const char kClientIdProduction[] = "";
const char kClientSecretStaging[] = "67bf87da096748c5bc1e195cfbdd59db006618a0";
const char kClientSecretProduction[] = "";


using FetchBalanceCallback = std::function<void(ledger::Result, double)>;

class Uphold {
 public:
  explicit Uphold(bat_ledger::LedgerImpl* ledger);

  ~Uphold();
  void StartContribution(const std::string &viewing_id,
                         ledger::ExternalWallet wallet);

  void FetchBalance(std::map<std::string, ledger::ExternalWallet> wallets,
                    FetchBalanceCallback callback);

  static ledger::ExternalWallet GetWallet(
      std::map<std::string, ledger::ExternalWallet> wallets);

  static std::string GetAPIUrl(const std::string& path);

 private:
  void OnFetchBalance(
    FetchBalanceCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  std::unique_ptr<UpholdContribution> contribution_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_H_

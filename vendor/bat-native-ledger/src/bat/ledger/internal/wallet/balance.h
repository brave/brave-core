/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_WALLET_BALANCE_H_
#define BRAVELEDGER_WALLET_BALANCE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {
class Uphold;
}

namespace braveledger_wallet {

class Balance {
 public:
  explicit Balance(bat_ledger::LedgerImpl* ledger);

  ~Balance();

  void Fetch(ledger::FetchBalanceCallback callback);

  static double GetPerWalletBalance(
      const std::string& type,
      base::flat_map<std::string, double> wallets);

 private:
  void OnWalletProperties(
      int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers,
      ledger::FetchBalanceCallback callback);

  void GetUnBlindedTokens(
      ledger::BalancePtr balance,
      ledger::FetchBalanceCallback callback);

  void OnGetUnBlindedTokens(
      ledger::Balance info,
      ledger::FetchBalanceCallback callback,
      ledger::UnblindedTokenList list);

  void ExternalWallets(
      ledger::BalancePtr balance,
      ledger::FetchBalanceCallback callback);

  void OnExternalWallets(
      ledger::Balance info,
      ledger::FetchBalanceCallback callback,
      std::map<std::string, ledger::ExternalWalletPtr> wallets);

  void OnUpholdFetchBalance(ledger::Balance info,
                            ledger::FetchBalanceCallback callback,
                            ledger::Result result,
                            double balance);

  std::unique_ptr<braveledger_uphold::Uphold> uphold_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_wallet
#endif  // BRAVELEDGER_WALLET_BALANCE_H_

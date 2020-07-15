/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_WALLET_WALLET_BALANCE_H_
#define BRAVELEDGER_WALLET_WALLET_BALANCE_H_

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

class WalletBalance {
 public:
  explicit WalletBalance(bat_ledger::LedgerImpl* ledger);
  ~WalletBalance();

  void Fetch(ledger::FetchBalanceCallback callback);

  static double GetPerWalletBalance(
      const std::string& type,
      base::flat_map<std::string, double> wallets);

 private:
  void OnFetch(
      const ledger::UrlResponse& response,
      ledger::FetchBalanceCallback callback);

  void GetUnblindedTokens(
      ledger::BalancePtr balance,
      ledger::FetchBalanceCallback callback);

  void OnGetUnblindedTokens(
      ledger::Balance info,
      ledger::FetchBalanceCallback callback,
      ledger::UnblindedTokenList list);

  void ExternalWallets(
      ledger::BalancePtr balance,
      ledger::FetchBalanceCallback callback);

  void OnUpholdFetchBalance(
      ledger::Balance info,
      ledger::FetchBalanceCallback callback,
      const ledger::Result result,
      const double balance);

  std::unique_ptr<braveledger_uphold::Uphold> uphold_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_wallet
#endif  // BRAVELEDGER_WALLET_WALLET_BALANCE_H_

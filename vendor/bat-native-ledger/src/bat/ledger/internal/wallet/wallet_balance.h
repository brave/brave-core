/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_BALANCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_BALANCE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace wallet {

class WalletBalance {
 public:
  explicit WalletBalance(LedgerImpl* ledger);
  ~WalletBalance();

  void Fetch(ledger::FetchBalanceCallback callback);

  static double GetPerWalletBalance(
      const std::string& type,
      base::flat_map<std::string, double> wallets);

 private:
  void OnFetch(
      const type::Result result,
      type::BalancePtr balance,
      ledger::FetchBalanceCallback callback);

  void GetUnblindedTokens(
      type::BalancePtr balance,
      ledger::FetchBalanceCallback callback);

  void OnGetUnblindedTokens(
      type::Balance info,
      ledger::FetchBalanceCallback callback,
      type::UnblindedTokenList list);

  void ExternalWallets(
      type::BalancePtr balance,
      ledger::FetchBalanceCallback callback);

  void FetchBalanceUphold(type::BalancePtr balance,
                          ledger::FetchBalanceCallback callback);

  void OnFetchBalanceUphold(type::Balance info,
                            ledger::FetchBalanceCallback callback,
                            const type::Result result,
                            const double balance);

  void FetchBalanceBitflyer(type::BalancePtr balance,
                            ledger::FetchBalanceCallback callback);

  void OnFetchBalanceBitflyer(type::Balance info,
                              ledger::FetchBalanceCallback callback,
                              const type::Result result,
                              const double balance);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace wallet
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_BALANCE_H_

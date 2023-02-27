/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_BALANCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_BALANCE_H_

#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace wallet {

class WalletBalance {
 public:
  explicit WalletBalance(LedgerImpl* ledger);
  ~WalletBalance();

  void Fetch(ledger::FetchBalanceCallback callback);

 private:
  void GetUnblindedTokens(ledger::FetchBalanceCallback callback);

  void OnGetUnblindedTokens(ledger::FetchBalanceCallback callback,
                            std::vector<mojom::UnblindedTokenPtr> list);

  void FetchExternalWalletBalance(mojom::BalancePtr balance,
                                  ledger::FetchBalanceCallback callback);

  void OnFetchExternalWalletBalance(const std::string& wallet_type,
                                    mojom::BalancePtr balance_ptr,
                                    ledger::FetchBalanceCallback callback,
                                    mojom::Result result,
                                    double balance);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace wallet
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_WALLET_BALANCE_H_

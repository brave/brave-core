/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_WALLET_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_WALLET_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/uphold/uphold.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_uphold {

class UpholdWallet {
 public:
  explicit UpholdWallet(bat_ledger::LedgerImpl* ledger, Uphold* uphold);

  ~UpholdWallet();

  void Generate(
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletCallback callback);

 private:
  void OnGenerate(
    const ledger::Result result,
    const User& user,
    const ledger::ExternalWallet& wallet,
    ledger::ExternalWalletCallback callback);

  void OnCreateCard(
    const ledger::ExternalWallet& wallet,
    ledger::ExternalWalletCallback callback,
    const ledger::Result result,
    const std::string& address);

  ledger::ExternalWalletPtr SetStatus(
    const User& user,
    ledger::ExternalWalletPtr wallet);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Uphold* uphold_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_WALLET_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_WALLET_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_WALLET_H_

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

  void Generate(ledger::ResultCallback callback);

 private:
  void OnGenerate(
      const ledger::Result result,
      const User& user,
      ledger::ResultCallback callback);

  void OnCreateCard(
      const ledger::Result result,
      const std::string& address,
      ledger::ResultCallback callback);

  ledger::WalletStatus GetNewStatus(
      const ledger::WalletStatus old_status,
      const User& user);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  Uphold* uphold_;  // NOT OWNED
};

}  // namespace braveledger_uphold
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_WALLET_H_

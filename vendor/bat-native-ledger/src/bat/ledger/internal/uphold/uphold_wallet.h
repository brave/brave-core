/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_UPHOLD_UPHOLD_WALLET_H_
#define BRAVELEDGER_UPHOLD_UPHOLD_WALLET_H_

#include <string>

#include "bat/ledger/internal/uphold/uphold_user.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace uphold {

class UpholdWallet {
 public:
  explicit UpholdWallet(LedgerImpl* ledger);

  ~UpholdWallet();

  void Generate(ledger::ResultCallback callback);

 private:
  void OnGetUser(
      const type::Result result,
      const User& user,
      ledger::ResultCallback callback);

  void OnCreateCard(
      const type::Result result,
      const std::string& address,
      ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVELEDGER_UPHOLD_UPHOLD_WALLET_H_

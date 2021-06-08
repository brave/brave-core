/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_WALLET_H_

#include <string>

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
  void OnGenerate(
      const type::Result result,
      const User& user,
      ledger::ResultCallback callback);

  void OnCreateCard(
      const type::Result result,
      const std::string& address,
      ledger::ResultCallback callback);

  void OnTransferTokens(
      const type::Result result,
      std::string drain_id,
      ledger::ResultCallback callback);

  type::WalletStatus GetNewStatus(
      const type::WalletStatus old_status,
      const User& user);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_WALLET_H_

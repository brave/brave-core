/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V10_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V10_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/endpoint/promotion/get_wallet/get_wallet.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace state {

class StateMigrationV10 {
 public:
  explicit StateMigrationV10(LedgerImpl* ledger);
  ~StateMigrationV10();

  void Migrate(ledger::ResultCallback callback);

 private:
  void OnGetWallet(type::Result result,
                   const std::string& custodian,
                   bool linked,
                   ledger::ResultCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<ledger::endpoint::promotion::GetWallet> get_wallet_;
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V10_H_

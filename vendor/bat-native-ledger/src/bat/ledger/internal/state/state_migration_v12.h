/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V12_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V12_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace state {

class StateMigrationV12 {
 public:
  explicit StateMigrationV12(LedgerImpl*);
  ~StateMigrationV12();

  void Migrate(ledger::LegacyResultCallback);

 private:
  bool MigrateExternalWallet(const std::string& wallet_type);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V12_H_

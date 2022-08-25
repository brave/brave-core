/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V12_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V12_H_

#include <memory>
#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace uphold {
class UpholdTransfer;
}

namespace state {

class StateMigrationV12 {
 public:
  explicit StateMigrationV12(LedgerImpl*);
  ~StateMigrationV12();

  void Migrate(ledger::LegacyResultCallback);

 private:
  void CreateTransactionForFee(ledger::LegacyResultCallback,
                               type::ExternalWalletPtr);

  void OnCreateTransactionForFee(ledger::LegacyResultCallback,
                                 const std::string& contribution_id,
                                 type::Result,
                                 const std::string& transaction_id);

  void OnSaveExternalTransactionForFee(ledger::LegacyResultCallback,
                                       type::Result);

  std::unique_ptr<uphold::UpholdTransfer> transfer_;
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_MIGRATION_V10_H_

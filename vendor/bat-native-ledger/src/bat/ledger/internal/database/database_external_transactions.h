/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_EXTERNAL_TRANSACTIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_EXTERNAL_TRANSACTIONS_H_

#include "bat/ledger/internal/database/database_table.h"

namespace ledger::database {

using GetExternalTransactionIdCallback =
    base::OnceCallback<void(absl::optional<std::string>)>;

class DatabaseExternalTransactions : public DatabaseTable {
 public:
  explicit DatabaseExternalTransactions(LedgerImpl*);
  ~DatabaseExternalTransactions() override;

  void Insert(type::ExternalTransactionPtr, ledger::ResultCallback);
  void GetExternalTransactionId(const std::string& contribution_id,
                                bool is_fee,
                                GetExternalTransactionIdCallback);

 private:
  void OnGetExternalTransactionId(GetExternalTransactionIdCallback,
                                  type::DBCommandResponsePtr response);
};

}  // namespace ledger::database

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_EXTERNAL_TRANSACTIONS_H_

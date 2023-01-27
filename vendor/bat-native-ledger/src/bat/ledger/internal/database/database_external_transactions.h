/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_EXTERNAL_TRANSACTIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_EXTERNAL_TRANSACTIONS_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger::database {

using GetExternalTransactionCallback = base::OnceCallback<void(mojom::ExternalTransactionPtr)>;

class DatabaseExternalTransactions : public DatabaseTable {
 public:
  explicit DatabaseExternalTransactions(LedgerImpl*);
  ~DatabaseExternalTransactions() override;

  void Insert(mojom::ExternalTransactionPtr, ledger::ResultCallback);
  void GetTransaction(const std::string& contribution_id,
                      const std::string& destination,
                      GetExternalTransactionCallback);

 private:
  void OnInsert(ledger::ResultCallback, mojom::DBCommandResponsePtr);
  void OnGetTransaction(GetExternalTransactionCallback,
                        mojom::DBCommandResponsePtr);
};

}  // namespace ledger::database

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_EXTERNAL_TRANSACTIONS_H_

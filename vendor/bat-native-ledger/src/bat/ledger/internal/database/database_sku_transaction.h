/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SKU_TRANSACTION_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SKU_TRANSACTION_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetSKUTransactionCallback = std::function<void(mojom::SKUTransactionPtr)>;

class DatabaseSKUTransaction: public DatabaseTable {
 public:
  explicit DatabaseSKUTransaction(LedgerImpl* ledger);
  ~DatabaseSKUTransaction() override;

  void InsertOrUpdate(mojom::SKUTransactionPtr info,
                      ledger::LegacyResultCallback callback);

  void SaveExternalTransaction(const std::string& transaction_id,
                               const std::string& external_transaction_id,
                               ledger::LegacyResultCallback callback);

  void GetRecordByOrderId(
      const std::string& order_id,
      GetSKUTransactionCallback callback);

 private:
  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   GetSKUTransactionCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_SKU_TRANSACTION_H_

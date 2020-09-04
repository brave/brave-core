/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_SKU_TRANSACTION_H_
#define BRAVELEDGER_DATABASE_DATABASE_SKU_TRANSACTION_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetSKUTransactionCallback = std::function<void(type::SKUTransactionPtr)>;

class DatabaseSKUTransaction: public DatabaseTable {
 public:
  explicit DatabaseSKUTransaction(LedgerImpl* ledger);
  ~DatabaseSKUTransaction() override;

  void InsertOrUpdate(
      type::SKUTransactionPtr info,
      ledger::ResultCallback callback);

  void SaveExternalTransaction(
      const std::string& transaction_id,
      const std::string& external_transaction_id,
      ledger::ResultCallback callback);

  void GetRecordByOrderId(
      const std::string& order_id,
      GetSKUTransactionCallback callback);

 private:
  void OnGetRecord(
      type::DBCommandResponsePtr response,
      GetSKUTransactionCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_SKU_TRANSACTION_H_

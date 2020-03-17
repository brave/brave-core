/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_CREDS_BATCH_H_
#define BRAVELEDGER_DATABASE_DATABASE_CREDS_BATCH_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseCredsBatch: public DatabaseTable {
 public:
  explicit DatabaseCredsBatch(bat_ledger::LedgerImpl* ledger);
  ~DatabaseCredsBatch() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      ledger::CredsBatchPtr creds,
      ledger::ResultCallback callback);

  void GetRecordByTrigger(
      const std::string& trigger_id,
      const ledger::CredsBatchType trigger_type,
      ledger::GetCredsBatchCallback callback);

  void SaveSignedCreds(
      ledger::CredsBatchPtr creds,
      ledger::ResultCallback callback);

  void GetAllRecords(ledger::GetAllCredsBatchCallback callback);

 private:
  bool CreateTableV18(ledger::DBTransaction* transaction);

  bool CreateIndexV18(ledger::DBTransaction* transaction);

  bool MigrateToV18(ledger::DBTransaction* transaction);

  void OnGetRecordByTrigger(
      ledger::DBCommandResponsePtr response,
      ledger::GetCredsBatchCallback callback);

  void OnGetAllRecords(
      ledger::DBCommandResponsePtr response,
      ledger::GetAllCredsBatchCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_CREDS_BATCH_H_

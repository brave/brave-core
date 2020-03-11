/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_PENDING_CONTRIBUTION_H_
#define BRAVELEDGER_DATABASE_DATABASE_PENDING_CONTRIBUTION_H_

#include <stdint.h>

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabasePendingContribution: public DatabaseTable {
 public:
  explicit DatabasePendingContribution(bat_ledger::LedgerImpl* ledger);
  ~DatabasePendingContribution() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdateList(
      ledger::PendingContributionList list,
      ledger::ResultCallback callback);

  void GetReservedAmount(ledger::PendingContributionsTotalCallback callback);

  void GetAllRecords(ledger::PendingContributionInfoListCallback callback);

  void DeleteRecord(const uint64_t id, ledger::ResultCallback callback);

  void DeleteAllRecords(ledger::ResultCallback callback);

 private:
  bool CreateTableV3(ledger::DBTransaction* transaction);

  bool CreateTableV8(ledger::DBTransaction* transaction);

  bool CreateTableV12(ledger::DBTransaction* transaction);

  bool CreateTableV15(ledger::DBTransaction* transaction);

  bool CreateIndexV3(ledger::DBTransaction* transaction);

  bool CreateIndexV8(ledger::DBTransaction* transaction);

  bool CreateIndexV12(ledger::DBTransaction* transaction);

  bool CreateIndexV15(ledger::DBTransaction* transaction);

  bool MigrateToV3(ledger::DBTransaction* transaction);

  bool MigrateToV8(ledger::DBTransaction* transaction);

  bool MigrateToV12(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);

  void OnGetReservedAmount(
      ledger::DBCommandResponsePtr response,
      ledger::PendingContributionsTotalCallback callback);

  void OnGetAllRecords(
      ledger::DBCommandResponsePtr response,
      ledger::PendingContributionInfoListCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_PENDING_CONTRIBUTION_H_

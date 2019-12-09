/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PENDING_CONTRIBUTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PENDING_CONTRIBUTION_H_

#include <stdint.h>

#include <string>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabasePendingContribution: public DatabaseTable {
 public:
  explicit DatabasePendingContribution(int current_db_version);

  ~DatabasePendingContribution() override;

  bool Init(sql::Database* db) override;

  bool CreateTable(sql::Database* db) override;

  bool CreateIndex(sql::Database* db) override;

  bool Migrate(sql::Database* db, const int target);

  bool InsertOrUpdate(sql::Database* db, ledger::PendingContributionList list);

  double GetReservedAmount(sql::Database* db);

  void GetAllRecords(
      sql::Database* db,
      ledger::PendingContributionInfoList* list);

  bool DeleteRecord(sql::Database* db, const uint64_t id);

  bool DeleteAllRecords(sql::Database* db);

 private:
  bool CreateTableV3(sql::Database* db);

  bool CreateTableV8(sql::Database* db);

  bool CreateTableV12(sql::Database* db);

  bool CreateIndexV3(sql::Database* db);

  bool CreateIndexV8(sql::Database* db);

  bool CreateIndexV12(sql::Database* db);

  bool MigrateToV3(sql::Database* db);

  bool MigrateToV8(sql::Database* db);

  bool MigrateToV12(sql::Database* db);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PENDING_CONTRIBUTION_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_ACTIVITY_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_ACTIVITY_INFO_H_

#include <string>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabaseActivityInfo: public DatabaseTable {
 public:
  explicit DatabaseActivityInfo(int current_db_version);

  ~DatabaseActivityInfo() override;

  bool Init(sql::Database* db) override;

  bool CreateTable(sql::Database* db) override;

  bool CreateIndex(sql::Database* db) override;

  bool Migrate(sql::Database* db, const int target);

  bool InsertOrUpdate(sql::Database* db, ledger::PublisherInfoPtr info);

  bool GetRecordsList(
      sql::Database* db,
      const int start,
      const int limit,
      ledger::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoList* list);

  bool DeleteRecord(
      sql::Database* db,
      const std::string& publisher_key,
      const uint64_t reconcile_stamp);

 private:
  bool CreateTableV1(sql::Database* db);

  bool CreateTableV2(sql::Database* db);

  bool CreateTableV4(sql::Database* db);

  bool CreateTableV6(sql::Database* db);

  bool CreateIndexV1(sql::Database* db);

  bool CreateIndexV2(sql::Database* db);

  bool CreateIndexV4(sql::Database* db);

  bool CreateIndexV6(sql::Database* db);

  bool MigrateToV2(sql::Database* db);

  bool MigrateToV4(sql::Database* db);

  bool MigrateToV5(sql::Database* db);

  bool MigrateToV6(sql::Database* db);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_ACTIVITY_INFO_H_

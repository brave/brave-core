/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PUBLISHER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PUBLISHER_INFO_H_

#include <string>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabasePublisherInfo: public DatabaseTable {
 public:
  explicit DatabasePublisherInfo(int current_db_version);

  ~DatabasePublisherInfo() override;

  bool Migrate(sql::Database* db, const int target) override;

  bool InsertOrUpdate(sql::Database* db, ledger::PublisherInfoPtr info);

  ledger::PublisherInfoPtr GetRecord(
      sql::Database* db,
      const std::string& publisher_key);

  ledger::PublisherInfoPtr GetPanelRecord(
      sql::Database* db,
      ledger::ActivityInfoFilterPtr filter);

  bool RestorePublishers(sql::Database* db);

  bool GetExcludedList(sql::Database* db, ledger::PublisherInfoList* list);

 private:
  bool CreateTableV1(sql::Database* db);

  bool CreateTableV7(sql::Database* db);

  bool MigrateToV1(sql::Database* db);

  bool MigrateToV7(sql::Database* db);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PUBLISHER_INFO_H_

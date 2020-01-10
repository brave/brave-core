/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_RECURRING_TIP_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_RECURRING_TIP_H_

#include <string>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabaseRecurringTip: public DatabaseTable {
 public:
  explicit DatabaseRecurringTip(int current_db_version);

  ~DatabaseRecurringTip() override;

  bool Migrate(sql::Database* db, const int target) override;

  bool InsertOrUpdate(sql::Database* db, ledger::RecurringTipPtr info);

  void GetAllRecords(
      sql::Database* db,
      ledger::PublisherInfoList* list);

  bool DeleteRecord(sql::Database* db, const std::string& publisher_key);

 private:
  bool CreateTableV2(sql::Database* db);

  bool CreateTableV15(sql::Database* db);

  bool CreateIndexV2(sql::Database* db);

  bool CreateIndexV15(sql::Database* db);

  bool MigrateToV2(sql::Database* db);

  bool MigrateToV15(sql::Database* db);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_RECURRING_TIP_H_

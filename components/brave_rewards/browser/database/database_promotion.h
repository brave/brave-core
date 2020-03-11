/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PROMOTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PROMOTION_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_promotion_creds.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabasePromotion: public DatabaseTable {
 public:
  explicit DatabasePromotion(int current_db_version);
  ~DatabasePromotion() override;

  bool Migrate(sql::Database* db, const int target) override;

  bool InsertOrUpdate(sql::Database* db, ledger::PromotionPtr info);

  ledger::PromotionPtr GetRecord(sql::Database* db, const std::string& id);

  ledger::PromotionMap GetAllRecords(sql::Database* db);

  bool DeleteRecordList(
      sql::Database* db,
      const std::vector<std::string>& id_list);

 private:
  bool CreateTableV10(sql::Database* db);

  bool CreateIndexV10(sql::Database* db);

  bool CreateTableV13(sql::Database* db);

  bool CreateIndexV13(sql::Database* db);

  bool MigrateToV10(sql::Database* db);

  bool MigrateToV13(sql::Database* db);

  bool MigrateToV14(sql::Database* db);

  bool MigrateToV15(sql::Database* db);

  std::unique_ptr<DatabasePromotionCreds> creds_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PROMOTION_H_

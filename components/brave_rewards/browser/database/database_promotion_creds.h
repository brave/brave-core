/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PROMOTION_CREDS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PROMOTION_CREDS_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabasePromotionCreds: public DatabaseTable {
 public:
  explicit DatabasePromotionCreds(int current_db_version);
  ~DatabasePromotionCreds() override;

  bool Migrate(sql::Database* db, const int target) override;

  bool InsertOrUpdate(
      sql::Database* db,
      ledger::PromotionCredsPtr info,
      const std::string& promotion_id);

  ledger::PromotionCredsPtr GetRecord(
      sql::Database* db,
      const std::string& promotion_id);

  bool DeleteRecordListByPromotion(
      sql::Database* db,
      const std::vector<std::string>& id_list);

 private:
  bool CreateTableV10(sql::Database* db);

  bool CreateIndexV10(sql::Database* db);

  bool CreateTableV15(sql::Database* db);

  bool CreateIndexV15(sql::Database* db);

  bool MigrateToV10(sql::Database* db);

  bool MigrateToV15(sql::Database* db);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_PROMOTION_CREDS_H_

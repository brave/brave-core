/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabaseUnblindedToken: public DatabaseTable {
 public:
  explicit DatabaseUnblindedToken(int current_db_version);
  ~DatabaseUnblindedToken() override;

  bool Init(sql::Database* db) override;

  bool CreateTable(sql::Database* db) override;

  bool CreateIndex(sql::Database* db) override;

  bool InsertOrUpdate(sql::Database* db, ledger::UnblindedTokenPtr info);

  ledger::UnblindedTokenList GetAllRecords(sql::Database* db);

  bool DeleteRecord(sql::Database* db, const std::vector<std::string>& id_list);

  static bool DeleteRecordsForPromotion(
      sql::Database* db,
      const std::string& promotion_id);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

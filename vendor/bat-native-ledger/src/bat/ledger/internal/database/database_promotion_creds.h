/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_PROMOTION_CREDS_H_
#define BRAVELEDGER_DATABASE_DATABASE_PROMOTION_CREDS_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabasePromotionCreds: public DatabaseTable {
 public:
  explicit DatabasePromotionCreds(bat_ledger::LedgerImpl* ledger);
  ~DatabasePromotionCreds() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      ledger::DBTransaction* transaction,
      ledger::PromotionCredsPtr info,
      const std::string& promotion_id);


  void DeleteRecordListByPromotion(
      ledger::DBTransaction* transaction,
      const std::vector<std::string>& ids);

 private:
  bool CreateTableV10(ledger::DBTransaction* transaction);

  bool CreateTableV15(ledger::DBTransaction* transaction);

  bool CreateIndexV10(ledger::DBTransaction* transaction);

  bool CreateIndexV15(ledger::DBTransaction* transaction);

  bool MigrateToV10(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_PROMOTION_CREDS_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_
#define BRAVELEDGER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseUnblindedToken: public DatabaseTable {
 public:
  explicit DatabaseUnblindedToken(bat_ledger::LedgerImpl* ledger);
  ~DatabaseUnblindedToken() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdateList(
      ledger::UnblindedTokenList list,
      ledger::ResultCallback callback);

  void GetAllRecords(ledger::GetUnblindedTokenListCallback callback);

  void DeleteRecordList(
      const std::vector<std::string>& ids,
      ledger::ResultCallback callback);

  void DeleteRecordsForPromotion(
      const std::string& promotion_id,
      ledger::ResultCallback callback);

  void GetRecordsByPromotionType(
      const std::vector<ledger::PromotionType>& promotion_types,
      ledger::GetUnblindedTokenListCallback callback);

 private:
  bool CreateTableV10(ledger::DBTransaction* transaction);

  bool CreateTableV15(ledger::DBTransaction* transaction);

  bool CreateIndexV10(ledger::DBTransaction* transaction);

  bool CreateIndexV15(ledger::DBTransaction* transaction);

  bool MigrateToV10(ledger::DBTransaction* transaction);

  bool MigrateToV14(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);

  void OnGetAllRecords(
      ledger::DBCommandResponsePtr response,
      ledger::GetUnblindedTokenListCallback callback);

  void OnGetRecordsByPromotionType(
      ledger::DBCommandResponsePtr response,
      ledger::GetUnblindedTokenListCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

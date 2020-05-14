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

  void GetSpendableRecordsByTriggerIds(
      const std::vector<std::string>& trigger_ids,
      ledger::GetUnblindedTokenListCallback callback);

  void MarkRecordListAsSpent(
      const std::vector<std::string>& ids,
      ledger::RewardsType redeem_type,
      const std::string& redeem_id,
      ledger::ResultCallback callback);

  void MarkRecordListAsReserved(
      const std::vector<std::string>& ids,
      const std::string& redeem_id,
      ledger::ResultCallback callback);

  void MarkRecordListAsSpendable(
      const std::string& redeem_id,
      ledger::ResultCallback callback);

  void GetReservedRecordList(
      const std::string& redeem_id,
      ledger::GetUnblindedTokenListCallback callback);

  void GetSpendableRecordListByBatchTypes(
      const std::vector<ledger::CredsBatchType>& batch_types,
      ledger::GetUnblindedTokenListCallback callback);

 private:
  bool CreateTableV10(ledger::DBTransaction* transaction);

  bool CreateTableV15(ledger::DBTransaction* transaction);

  bool CreateTableV18(ledger::DBTransaction* transaction);

  bool CreateTableV26(ledger::DBTransaction* transaction);

  bool CreateIndexV10(ledger::DBTransaction* transaction);

  bool CreateIndexV15(ledger::DBTransaction* transaction);

  bool CreateIndexV18(ledger::DBTransaction* transaction);

  bool CreateIndexV20(ledger::DBTransaction* transaction);

  bool CreateIndexV26(ledger::DBTransaction* transaction);

  bool MigrateToV10(ledger::DBTransaction* transaction);

  bool MigrateToV14(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);

  bool MigrateToV18(ledger::DBTransaction* transaction);

  bool MigrateToV20(ledger::DBTransaction* transaction);

  bool MigrateToV27(ledger::DBTransaction* transaction);

  bool MigrateToV26(ledger::DBTransaction* transaction);

  void OnGetRecords(
      ledger::DBCommandResponsePtr response,
      ledger::GetUnblindedTokenListCallback callback);

  void OnMarkRecordListAsReserved(
      ledger::DBCommandResponsePtr response,
      size_t expected_row_count,
      ledger::ResultCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

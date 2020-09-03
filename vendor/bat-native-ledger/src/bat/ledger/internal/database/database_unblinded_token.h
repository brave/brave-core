/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_
#define BRAVELEDGER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetUnblindedTokenListCallback =
    std::function<void(type::UnblindedTokenList)>;

class DatabaseUnblindedToken: public DatabaseTable {
 public:
  explicit DatabaseUnblindedToken(LedgerImpl* ledger);
  ~DatabaseUnblindedToken() override;

  void InsertOrUpdateList(
      type::UnblindedTokenList list,
      ledger::ResultCallback callback);

  void GetSpendableRecordsByTriggerIds(
      const std::vector<std::string>& trigger_ids,
      GetUnblindedTokenListCallback callback);

  void MarkRecordListAsSpent(
      const std::vector<std::string>& ids,
      type::RewardsType redeem_type,
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
      GetUnblindedTokenListCallback callback);

  void GetSpendableRecordListByBatchTypes(
      const std::vector<type::CredsBatchType>& batch_types,
      GetUnblindedTokenListCallback callback);

 private:
  void OnGetRecords(
      type::DBCommandResponsePtr response,
      GetUnblindedTokenListCallback callback);

  void OnMarkRecordListAsReserved(
      type::DBCommandResponsePtr response,
      size_t expected_row_count,
      ledger::ResultCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

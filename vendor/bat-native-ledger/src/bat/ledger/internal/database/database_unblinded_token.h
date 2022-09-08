/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_UNBLINDED_TOKEN_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetUnblindedTokenListCallback =
    std::function<void(std::vector<mojom::UnblindedTokenPtr>)>;

class DatabaseUnblindedToken: public DatabaseTable {
 public:
  explicit DatabaseUnblindedToken(LedgerImpl* ledger);
  ~DatabaseUnblindedToken() override;

  void InsertOrUpdateList(std::vector<mojom::UnblindedTokenPtr> list,
                          ledger::LegacyResultCallback callback);

  void GetSpendableRecords(GetUnblindedTokenListCallback callback);

  void MarkRecordListAsSpent(const std::vector<std::string>& ids,
                             mojom::RewardsType redeem_type,
                             const std::string& redeem_id,
                             ledger::LegacyResultCallback callback);

  void MarkRecordListAsReserved(const std::vector<std::string>& ids,
                                const std::string& redeem_id,
                                ledger::LegacyResultCallback callback);

  void MarkRecordListAsSpendable(const std::string& redeem_id,
                                 ledger::LegacyResultCallback callback);

  void GetReservedRecordList(
      const std::string& redeem_id,
      GetUnblindedTokenListCallback callback);

  void GetSpendableRecordListByBatchTypes(
      const std::vector<mojom::CredsBatchType>& batch_types,
      GetUnblindedTokenListCallback callback);

 private:
  void OnGetRecords(mojom::DBCommandResponsePtr response,
                    GetUnblindedTokenListCallback callback);

  void OnMarkRecordListAsReserved(mojom::DBCommandResponsePtr response,
                                  size_t expected_row_count,
                                  ledger::LegacyResultCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

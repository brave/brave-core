/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CREDS_BATCH_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CREDS_BATCH_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetCredsBatchCallback = std::function<void(mojom::CredsBatchPtr)>;
using GetCredsBatchListCallback =
    std::function<void(std::vector<mojom::CredsBatchPtr>)>;

class DatabaseCredsBatch: public DatabaseTable {
 public:
  explicit DatabaseCredsBatch(LedgerImpl* ledger);
  ~DatabaseCredsBatch() override;

  void InsertOrUpdate(mojom::CredsBatchPtr creds,
                      ledger::LegacyResultCallback callback);

  void GetRecordByTrigger(const std::string& trigger_id,
                          const mojom::CredsBatchType trigger_type,
                          GetCredsBatchCallback callback);

  void SaveSignedCreds(mojom::CredsBatchPtr creds,
                       ledger::LegacyResultCallback callback);

  void GetAllRecords(GetCredsBatchListCallback callback);

  void UpdateStatus(const std::string& trigger_id,
                    mojom::CredsBatchType trigger_type,
                    mojom::CredsBatchStatus status,
                    ledger::LegacyResultCallback callback);

  void UpdateRecordsStatus(const std::vector<std::string>& trigger_ids,
                           mojom::CredsBatchType trigger_type,
                           mojom::CredsBatchStatus status,
                           ledger::LegacyResultCallback callback);

  void GetRecordsByTriggers(
      const std::vector<std::string>& trigger_ids,
      GetCredsBatchListCallback callback);

 private:
  void OnGetRecordByTrigger(mojom::DBCommandResponsePtr response,
                            GetCredsBatchCallback callback);

  void OnGetRecords(mojom::DBCommandResponsePtr response,
                    GetCredsBatchListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CREDS_BATCH_H_

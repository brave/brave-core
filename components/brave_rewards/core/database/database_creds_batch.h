/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CREDS_BATCH_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CREDS_BATCH_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

using GetCredsBatchCallback = std::function<void(mojom::CredsBatchPtr)>;
using GetCredsBatchListCallback =
    std::function<void(std::vector<mojom::CredsBatchPtr>)>;

class DatabaseCredsBatch : public DatabaseTable {
 public:
  explicit DatabaseCredsBatch(RewardsEngineImpl& engine);
  ~DatabaseCredsBatch() override;

  void InsertOrUpdate(mojom::CredsBatchPtr creds,
                      LegacyResultCallback callback);

  void GetRecordByTrigger(const std::string& trigger_id,
                          const mojom::CredsBatchType trigger_type,
                          GetCredsBatchCallback callback);

  void SaveSignedCreds(mojom::CredsBatchPtr creds,
                       LegacyResultCallback callback);

  void GetAllRecords(GetCredsBatchListCallback callback);

  void UpdateStatus(const std::string& trigger_id,
                    mojom::CredsBatchType trigger_type,
                    mojom::CredsBatchStatus status,
                    LegacyResultCallback callback);

  void UpdateRecordsStatus(const std::vector<std::string>& trigger_ids,
                           mojom::CredsBatchType trigger_type,
                           mojom::CredsBatchStatus status,
                           LegacyResultCallback callback);

  void GetRecordsByTriggers(const std::vector<std::string>& trigger_ids,
                            GetCredsBatchListCallback callback);

 private:
  void OnGetRecordByTrigger(mojom::DBCommandResponsePtr response,
                            GetCredsBatchCallback callback);

  void OnGetRecords(mojom::DBCommandResponsePtr response,
                    GetCredsBatchListCallback callback);
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CREDS_BATCH_H_

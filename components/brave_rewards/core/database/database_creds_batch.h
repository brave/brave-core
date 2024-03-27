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

using GetCredsBatchCallback = base::OnceCallback<void(mojom::CredsBatchPtr)>;
using GetCredsBatchListCallback =
    base::OnceCallback<void(std::vector<mojom::CredsBatchPtr>)>;

class DatabaseCredsBatch : public DatabaseTable {
 public:
  explicit DatabaseCredsBatch(RewardsEngine& engine);
  ~DatabaseCredsBatch() override;

  void InsertOrUpdate(mojom::CredsBatchPtr creds, ResultCallback callback);

  void GetRecordByTrigger(const std::string& trigger_id,
                          const mojom::CredsBatchType trigger_type,
                          GetCredsBatchCallback callback);

  void SaveSignedCreds(mojom::CredsBatchPtr creds, ResultCallback callback);

  void GetAllRecords(GetCredsBatchListCallback callback);

  void UpdateStatus(const std::string& trigger_id,
                    mojom::CredsBatchType trigger_type,
                    mojom::CredsBatchStatus status,
                    ResultCallback callback);

  void UpdateRecordsStatus(const std::vector<std::string>& trigger_ids,
                           mojom::CredsBatchType trigger_type,
                           mojom::CredsBatchStatus status,
                           ResultCallback callback);

  void GetRecordsByTriggers(const std::vector<std::string>& trigger_ids,
                            GetCredsBatchListCallback callback);

 private:
  void OnGetRecordByTrigger(GetCredsBatchCallback callback,
                            mojom::DBCommandResponsePtr response);

  void OnGetRecords(GetCredsBatchListCallback callback,
                    mojom::DBCommandResponsePtr response);
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CREDS_BATCH_H_

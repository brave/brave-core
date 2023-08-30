/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_UNBLINDED_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

using GetUnblindedTokenListCallback =
    std::function<void(std::vector<mojom::UnblindedTokenPtr>)>;

class DatabaseUnblindedToken : public DatabaseTable {
 public:
  explicit DatabaseUnblindedToken(RewardsEngineImpl& engine);
  ~DatabaseUnblindedToken() override;

  void InsertOrUpdateList(std::vector<mojom::UnblindedTokenPtr> list,
                          LegacyResultCallback callback);

  void GetSpendableRecords(GetUnblindedTokenListCallback callback);

  void MarkRecordListAsSpent(const std::vector<std::string>& ids,
                             mojom::RewardsType redeem_type,
                             const std::string& redeem_id,
                             LegacyResultCallback callback);

  void MarkRecordListAsReserved(const std::vector<std::string>& ids,
                                const std::string& redeem_id,
                                LegacyResultCallback callback);

  void MarkRecordListAsSpendable(const std::string& redeem_id,
                                 LegacyResultCallback callback);

  void GetReservedRecordList(const std::string& redeem_id,
                             GetUnblindedTokenListCallback callback);

  void GetSpendableRecordListByBatchTypes(
      const std::vector<mojom::CredsBatchType>& batch_types,
      GetUnblindedTokenListCallback callback);

 private:
  void OnGetRecords(GetUnblindedTokenListCallback callback,
                    mojom::DBCommandResponsePtr response);

  void OnMarkRecordListAsReserved(LegacyResultCallback callback,
                                  size_t expected_row_count,
                                  mojom::DBCommandResponsePtr response);
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_UNBLINDED_TOKEN_H_

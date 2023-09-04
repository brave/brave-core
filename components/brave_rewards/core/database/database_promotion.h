/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PROMOTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PROMOTION_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

using GetPromotionCallback = std::function<void(mojom::PromotionPtr)>;

using GetPromotionListCallback =
    std::function<void(std::vector<mojom::PromotionPtr>)>;

class DatabasePromotion : public DatabaseTable {
 public:
  explicit DatabasePromotion(RewardsEngineImpl& engine);
  ~DatabasePromotion() override;

  void InsertOrUpdate(mojom::PromotionPtr info, LegacyResultCallback callback);

  void GetRecord(const std::string& id, GetPromotionCallback callback);

  void GetRecords(const std::vector<std::string>& ids,
                  GetPromotionListCallback callback);

  void GetAllRecords(GetAllPromotionsCallback callback);

  void SaveClaimId(const std::string& promotion_id,
                   const std::string& claim_id,
                   LegacyResultCallback callback);

  void UpdateStatus(const std::string& promotion_id,
                    mojom::PromotionStatus status,
                    LegacyResultCallback callback);

  void UpdateRecordsStatus(const std::vector<std::string>& ids,
                           mojom::PromotionStatus status,
                           LegacyResultCallback callback);

  void CredentialCompleted(const std::string& promotion_id,
                           LegacyResultCallback callback);

  void UpdateRecordsBlankPublicKey(const std::vector<std::string>& ids,
                                   LegacyResultCallback callback);

 private:
  void OnGetRecord(GetPromotionCallback callback,
                   mojom::DBCommandResponsePtr response);

  void OnGetAllRecords(GetAllPromotionsCallback callback,
                       mojom::DBCommandResponsePtr response);

  void OnGetRecords(GetPromotionListCallback callback,
                    mojom::DBCommandResponsePtr response);
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PROMOTION_H_

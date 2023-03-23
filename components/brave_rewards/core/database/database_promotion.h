/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PROMOTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PROMOTION_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace ledger {
namespace database {

using GetPromotionCallback = std::function<void(mojom::PromotionPtr)>;

using GetPromotionListCallback =
    std::function<void(std::vector<mojom::PromotionPtr>)>;

class DatabasePromotion : public DatabaseTable {
 public:
  explicit DatabasePromotion(LedgerImpl* ledger);
  ~DatabasePromotion() override;

  void InsertOrUpdate(mojom::PromotionPtr info,
                      ledger::LegacyResultCallback callback);

  void GetRecord(const std::string& id, GetPromotionCallback callback);

  void GetRecords(const std::vector<std::string>& ids,
                  GetPromotionListCallback callback);

  void GetAllRecords(ledger::GetAllPromotionsCallback callback);

  void SaveClaimId(const std::string& promotion_id,
                   const std::string& claim_id,
                   ledger::LegacyResultCallback callback);

  void UpdateStatus(const std::string& promotion_id,
                    mojom::PromotionStatus status,
                    ledger::LegacyResultCallback callback);

  void UpdateRecordsStatus(const std::vector<std::string>& ids,
                           mojom::PromotionStatus status,
                           ledger::LegacyResultCallback callback);

  void CredentialCompleted(const std::string& promotion_id,
                           ledger::LegacyResultCallback callback);

  void UpdateRecordsBlankPublicKey(const std::vector<std::string>& ids,
                                   ledger::LegacyResultCallback callback);

 private:
  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   GetPromotionCallback callback);

  void OnGetAllRecords(mojom::DBCommandResponsePtr response,
                       ledger::GetAllPromotionsCallback callback);

  void OnGetRecords(mojom::DBCommandResponsePtr response,
                    GetPromotionListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PROMOTION_H_

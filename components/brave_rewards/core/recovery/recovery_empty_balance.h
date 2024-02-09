/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_RECOVERY_RECOVERY_EMPTY_BALANCE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_RECOVERY_RECOVERY_EMPTY_BALANCE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/database/database_promotion.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotion_server.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace recovery {

class EmptyBalance {
 public:
  explicit EmptyBalance(RewardsEngineImpl& engine);
  ~EmptyBalance();

  void Check();

 private:
  void OnAllContributions(std::vector<mojom::ContributionInfoPtr> list);

  void GetPromotions(database::GetPromotionListCallback callback);

  void OnPromotions(
      database::GetPromotionListCallback callback,
      base::flat_map<std::string, mojom::PromotionPtr> promotions);

  void GetCredsByPromotions(std::vector<mojom::PromotionPtr> list);

  void OnCreds(std::vector<mojom::CredsBatchPtr> list);

  void OnSaveUnblindedCreds(const mojom::Result result);

  void GetAllTokens(double contribution_sum,
                    std::vector<mojom::PromotionPtr> list);

  void ReportResults(double contribution_sum,
                     double promotion_sum,
                     std::vector<mojom::UnblindedTokenPtr> list);

  void Sent(mojom::Result result);

  const raw_ref<RewardsEngineImpl> engine_;
  endpoint::PromotionServer promotion_server_;
  base::WeakPtrFactory<EmptyBalance> weak_factory_{this};
};

}  // namespace recovery
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_RECOVERY_RECOVERY_EMPTY_BALANCE_H_

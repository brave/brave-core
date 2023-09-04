/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MULTI_TABLES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MULTI_TABLES_H_

#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace database {

class DatabaseMultiTables {
 public:
  explicit DatabaseMultiTables(RewardsEngineImpl& engine);
  ~DatabaseMultiTables();

  void GetTransactionReport(const mojom::ActivityMonth month,
                            const int year,
                            GetTransactionReportCallback callback);

 private:
  void OnGetTransactionReportPromotion(
      base::flat_map<std::string, mojom::PromotionPtr> promotions,
      const mojom::ActivityMonth month,
      const int year,
      GetTransactionReportCallback callback);

  const raw_ref<RewardsEngineImpl> engine_;
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MULTI_TABLES_H_

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_DATABASE_TABLE_H_

#include <string>

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads::database::table {

class AdHistory final : public TableInterface {
 public:
  AdHistory();

  void Save(const AdHistoryList& ad_history, ResultCallback callback) const;

  void GetForDateRange(base::Time from_time,
                       base::Time to_time,
                       GetAdHistoryCallback callback) const;
  void GetHighestRankedPlacementsForDateRange(
      base::Time from_time,
      base::Time to_time,
      GetAdHistoryCallback callback) const;
  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetAdHistoryCallback callback) const;

  void PurgeExpired(ResultCallback callback) const;

  void SetBatchSize(const int batch_size) {
    CHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              const AdHistoryList& ad_history) const;

  std::string BuildInsertSql(const mojom::DBActionInfoPtr& mojom_db_action,
                             const AdHistoryList& ad_history) const;

  int batch_size_;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_DATABASE_TABLE_H_

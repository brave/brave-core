/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NOTIFICATION_ADS_CREATIVE_NOTIFICATION_ADS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NOTIFICATION_ADS_CREATIVE_NOTIFICATION_ADS_DATABASE_TABLE_H_

#include <cstddef>
#include <string>

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads::database::table {

using GetCreativeNotificationAdsCallback =
    base::OnceCallback<void(bool success,
                            const SegmentList& segments,
                            CreativeNotificationAdList creative_ads)>;

class CreativeNotificationAds final : public TableInterface {
 public:
  CreativeNotificationAds();

  CreativeNotificationAds(const CreativeNotificationAds&) = delete;
  CreativeNotificationAds& operator=(const CreativeNotificationAds&) = delete;

  ~CreativeNotificationAds() override;

  void Save(const CreativeNotificationAdList& creative_ads,
            ResultCallback callback);

  void GetForSegments(const SegmentList& segments,
                      GetCreativeNotificationAdsCallback callback) const;

  void GetForActiveCampaigns(GetCreativeNotificationAdsCallback callback) const;

  void SetBatchSize(int batch_size) {
    CHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  void MigrateToV48(const mojom::DBTransactionInfoPtr& mojom_db_transaction);

  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              const CreativeNotificationAdList& creative_ads);

  std::string BuildInsertSql(
      const mojom::DBActionInfoPtr& mojom_db_action,
      const CreativeNotificationAdList& creative_ads) const;

  int batch_size_;

  Campaigns campaigns_database_table_;
  CreativeAds creative_ads_database_table_;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NOTIFICATION_ADS_CREATIVE_NOTIFICATION_ADS_DATABASE_TABLE_H_

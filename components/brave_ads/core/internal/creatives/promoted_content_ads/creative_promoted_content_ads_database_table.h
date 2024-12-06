/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_H_

#include <string>
#include <vector>

#include "base/check_op.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/geo_targets_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/segments_database_table.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

namespace brave_ads::database::table {

using GetCreativePromotedContentAdCallback =
    base::OnceCallback<void(bool success,
                            const std::string& creative_instance_id,
                            const CreativePromotedContentAdInfo& creative_ad)>;

using GetCreativePromotedContentAdsCallback =
    base::OnceCallback<void(bool success,
                            const std::vector<std::string>& segments,
                            const CreativePromotedContentAdList& creative_ads)>;

class CreativePromotedContentAds final : public TableInterface {
 public:
  CreativePromotedContentAds();

  CreativePromotedContentAds(const CreativePromotedContentAds&) = delete;
  CreativePromotedContentAds& operator=(const CreativePromotedContentAds&) =
      delete;

  ~CreativePromotedContentAds() override;

  void Save(const CreativePromotedContentAdList& creative_ads,
            ResultCallback callback);

  void Delete(ResultCallback callback) const;

  void GetForCreativeInstanceId(
      const std::string& creative_instance_id,
      GetCreativePromotedContentAdCallback callback) const;

  void GetForSegments(const SegmentList& segments,
                      GetCreativePromotedContentAdsCallback callback) const;

  void GetForActiveCampaigns(
      GetCreativePromotedContentAdsCallback callback) const;

  void SetBatchSize(int batch_size) {
    CHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  void MigrateToV45(const mojom::DBTransactionInfoPtr& mojom_db_transaction);

  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              const CreativePromotedContentAdList& creative_ads);

  std::string BuildInsertSql(
      const mojom::DBActionInfoPtr& mojom_db_action,
      const CreativePromotedContentAdList& creative_ads) const;

  int batch_size_;

  Campaigns campaigns_database_table_;
  CreativeAds creative_ads_database_table_;
  Dayparts dayparts_database_table_;
  Deposits deposits_database_table_;
  GeoTargets geo_targets_database_table_;
  Segments segments_database_table_;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_H_

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_

#include <string>

#include "base/check_op.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/geo_targets_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/segments_database_table.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

namespace brave_ads::database::table {

using GetCreativeInlineContentAdCallback =
    base::OnceCallback<void(bool success,
                            const std::string& creative_instance_id,
                            const CreativeInlineContentAdInfo& creative_ad)>;

using GetCreativeInlineContentAdsCallback =
    base::OnceCallback<void(bool success,
                            const SegmentList& segments,
                            const CreativeInlineContentAdList& creative_ads)>;

using GetCreativeInlineContentAdsForDimensionsCallback =
    base::OnceCallback<void(bool success,
                            const CreativeInlineContentAdList& creative_ads)>;

class CreativeInlineContentAds final : public TableInterface {
 public:
  CreativeInlineContentAds();

  CreativeInlineContentAds(const CreativeInlineContentAds&) = delete;
  CreativeInlineContentAds& operator=(const CreativeInlineContentAds&) = delete;

  CreativeInlineContentAds(CreativeInlineContentAds&&) noexcept = delete;
  CreativeInlineContentAds& operator=(CreativeInlineContentAds&&) noexcept =
      delete;

  ~CreativeInlineContentAds() override;

  void Save(const CreativeInlineContentAdList& creative_ads,
            ResultCallback callback);

  void Delete(ResultCallback callback) const;

  void GetForCreativeInstanceId(
      const std::string& creative_instance_id,
      GetCreativeInlineContentAdCallback callback) const;

  void GetForSegmentsAndDimensions(
      const SegmentList& segments,
      const std::string& dimensions,
      GetCreativeInlineContentAdsCallback callback) const;

  void GetForDimensions(
      const std::string& dimensions,
      GetCreativeInlineContentAdsForDimensionsCallback callback) const;

  void GetForActiveCampaigns(
      GetCreativeInlineContentAdsCallback callback) const;

  void SetBatchSize(const int batch_size) {
    CHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Create(mojom::DBTransactionInfo* mojom_db_transaction) override;
  void Migrate(mojom::DBTransactionInfo* mojom_db_transaction,
               int to_version) override;

 private:
  void MigrateToV43(mojom::DBTransactionInfo* mojom_db_transaction);

  void Insert(mojom::DBTransactionInfo* mojom_db_transaction,
              const CreativeInlineContentAdList& creative_ads);

  std::string BuildInsertSql(
      mojom::DBActionInfo* mojom_db_action,
      const CreativeInlineContentAdList& creative_ads) const;

  int batch_size_;

  Campaigns campaigns_database_table_;
  CreativeAds creative_ads_database_table_;
  Dayparts dayparts_database_table_;
  Deposits deposits_database_table_;
  GeoTargets geo_targets_database_table_;
  Segments segments_database_table_;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_

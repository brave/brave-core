/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_

#include <string>

#include "base/check_op.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads::database::table {

using GetCreativeInlineContentAdCallback =
    base::OnceCallback<void(bool success,
                            const std::string& creative_instance_id,
                            const CreativeInlineContentAdInfo& creative_ad)>;

using GetCreativeInlineContentAdsCallback =
    base::OnceCallback<void(bool success,
                            const SegmentList& segments,
                            CreativeInlineContentAdList creative_ads)>;

using GetCreativeInlineContentAdsForDimensionsCallback =
    base::OnceCallback<void(bool success,
                            CreativeInlineContentAdList creative_ads)>;

class CreativeInlineContentAds final : public TableInterface {
 public:
  CreativeInlineContentAds();

  CreativeInlineContentAds(const CreativeInlineContentAds&) = delete;
  CreativeInlineContentAds& operator=(const CreativeInlineContentAds&) = delete;

  ~CreativeInlineContentAds() override;

  void Save(const CreativeInlineContentAdList& creative_ads,
            ResultCallback callback);

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
              const CreativeInlineContentAdList& creative_ads);

  std::string BuildInsertSql(
      const mojom::DBActionInfoPtr& mojom_db_action,
      const CreativeInlineContentAdList& creative_ads) const;

  int batch_size_;

  Campaigns campaigns_database_table_;
  CreativeAds creative_ads_database_table_;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_

#include <memory>
#include <string>

#include "base/check_op.h"
#include "base/functional/callback_forward.h"
#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/internal/segments/segment_alias.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads::database::table {

class Campaigns;
class CreativeAds;
class Dayparts;
class Deposits;
class GeoTargets;
class Segments;

using GetCreativeInlineContentAdCallback =
    base::OnceCallback<void(const bool success,
                            const std::string& creative_instance_id,
                            const CreativeInlineContentAdInfo& creative_ad)>;

using GetCreativeInlineContentAdsCallback =
    base::OnceCallback<void(const bool success,
                            const SegmentList& segments,
                            const CreativeInlineContentAdList& creative_ads)>;

using GetCreativeInlineContentAdsForDimensionsCallback =
    base::OnceCallback<void(const bool success,
                            const CreativeInlineContentAdList& creative_ads)>;

class CreativeInlineContentAds final : public TableInterface {
 public:
  CreativeInlineContentAds();

  CreativeInlineContentAds(const CreativeInlineContentAds& other) = delete;
  CreativeInlineContentAds& operator=(const CreativeInlineContentAds& other) =
      delete;

  CreativeInlineContentAds(CreativeInlineContentAds&& other) noexcept = delete;
  CreativeInlineContentAds& operator=(
      CreativeInlineContentAds&& other) noexcept = delete;

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

  void GetAll(GetCreativeInlineContentAdsCallback callback) const;

  void SetBatchSize(const int batch_size) {
    DCHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const CreativeInlineContentAdList& creative_ads);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommandInfo* command,
      const CreativeInlineContentAdList& creative_ads) const;

  int batch_size_;

  std::unique_ptr<Campaigns> campaigns_database_table_;
  std::unique_ptr<CreativeAds> creative_ads_database_table_;
  std::unique_ptr<Dayparts> dayparts_database_table_;
  std::unique_ptr<Deposits> deposits_database_table_;
  std::unique_ptr<GeoTargets> geo_targets_database_table_;
  std::unique_ptr<Segments> segments_database_table_;
};

}  // namespace ads::database::table

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_

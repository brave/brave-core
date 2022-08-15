/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/check_op.h"
#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/internal/segments/segment_alias.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {
namespace table {

class Campaigns;
class CreativeAds;
class Dayparts;
class Deposits;
class GeoTargets;
class Segments;

using GetCreativePromotedContentAdCallback =
    std::function<void(const bool,
                       const std::string& creative_instance_id,
                       const CreativePromotedContentAdInfo&)>;

using GetCreativePromotedContentAdsCallback =
    std::function<void(const bool,
                       const std::vector<std::string>&,
                       const CreativePromotedContentAdList&)>;

class CreativePromotedContentAds final : public TableInterface {
 public:
  CreativePromotedContentAds();
  ~CreativePromotedContentAds() override;
  CreativePromotedContentAds(const CreativePromotedContentAds&) = delete;
  CreativePromotedContentAds& operator=(const CreativePromotedContentAds&) =
      delete;

  void Save(const CreativePromotedContentAdList& creative_promoted_content_ads,
            ResultCallback callback);

  void Delete(ResultCallback callback);

  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetCreativePromotedContentAdCallback callback);

  void GetForSegments(const SegmentList& segments,
                      GetCreativePromotedContentAdsCallback callback);

  void GetAll(GetCreativePromotedContentAdsCallback callback);

  void SetBatchSize(const int batch_size) {
    DCHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction,
               const int to_version) override;

 private:
  void InsertOrUpdate(
      mojom::DBTransactionInfo* transaction,
      const CreativePromotedContentAdList& creative_promoted_content_ads);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommandInfo* command,
      const CreativePromotedContentAdList& creative_promoted_content_ads);

  void OnGetForCreativeInstanceId(
      mojom::DBCommandResponseInfoPtr response,
      const std::string& creative_instance_id,
      GetCreativePromotedContentAdCallback callback);

  void OnGetForSegments(mojom::DBCommandResponseInfoPtr response,
                        const SegmentList& segments,
                        GetCreativePromotedContentAdsCallback callback);

  void OnGetAll(mojom::DBCommandResponseInfoPtr response,
                GetCreativePromotedContentAdsCallback callback);

  void MigrateToV24(mojom::DBTransactionInfo* transaction);

  int batch_size_;

  std::unique_ptr<Campaigns> campaigns_database_table_;
  std::unique_ptr<CreativeAds> creative_ads_database_table_;
  std::unique_ptr<Dayparts> dayparts_database_table_;
  std::unique_ptr<Deposits> deposits_database_table_;
  std::unique_ptr<GeoTargets> geo_targets_database_table_;
  std::unique_ptr<Segments> segments_database_table_;
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_PROMOTED_CONTENT_ADS_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_H_

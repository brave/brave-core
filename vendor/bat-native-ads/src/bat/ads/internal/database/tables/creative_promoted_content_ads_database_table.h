/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_H_

#include <memory>
#include <string>

#include "base/check_op.h"
#include "bat/ads/ads_client_aliases.h"
#include "bat/ads/internal/bundle/creative_promoted_content_ad_info_aliases.h"
#include "bat/ads/internal/database/database_table.h"
#include "bat/ads/internal/database/tables/creative_promoted_content_ads_database_table_aliases.h"
#include "bat/ads/internal/segments/segments_aliases.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct CreativePromotedContentAdInfo;

namespace database {
namespace table {

class Campaigns;
class CreativeAds;
class Dayparts;
class GeoTargets;
class Segments;

class CreativePromotedContentAds final : public Table {
 public:
  CreativePromotedContentAds();
  ~CreativePromotedContentAds() override;

  void Save(const CreativePromotedContentAdList& creative_promoted_content_ads,
            ResultCallback callback);

  void Delete(ResultCallback callback);

  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetCreativePromotedContentAdCallback callback);

  void GetForSegments(const SegmentList& segments,
                      GetCreativePromotedContentAdsCallback callback);

  void GetAll(GetCreativePromotedContentAdsCallback callback);

  void set_batch_size(const int batch_size) {
    DCHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransaction* transaction,
               const int to_version) override;

 private:
  void InsertOrUpdate(
      mojom::DBTransaction* transaction,
      const CreativePromotedContentAdList& creative_promoted_content_ads);

  int BindParameters(
      mojom::DBCommand* command,
      const CreativePromotedContentAdList& creative_promoted_content_ads);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommand* command,
      const CreativePromotedContentAdList& creative_promoted_content_ads);

  void OnGetForCreativeInstanceId(
      mojom::DBCommandResponsePtr response,
      const std::string& creative_instance_id,
      GetCreativePromotedContentAdCallback callback);

  void OnGetForSegments(mojom::DBCommandResponsePtr response,
                        const SegmentList& segments,
                        GetCreativePromotedContentAdsCallback callback);

  void OnGetAll(mojom::DBCommandResponsePtr response,
                GetCreativePromotedContentAdsCallback callback);

  CreativePromotedContentAdInfo GetFromRecord(mojom::DBRecord* record) const;

  void CreateTableV16(mojom::DBTransaction* transaction);
  void MigrateToV16(mojom::DBTransaction* transaction);

  int batch_size_;

  std::unique_ptr<Campaigns> campaigns_database_table_;
  std::unique_ptr<CreativeAds> creative_ads_database_table_;
  std::unique_ptr<Dayparts> dayparts_database_table_;
  std::unique_ptr<GeoTargets> geo_targets_database_table_;
  std::unique_ptr<Segments> segments_database_table_;
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_PROMOTED_CONTENT_ADS_DATABASE_TABLE_H_

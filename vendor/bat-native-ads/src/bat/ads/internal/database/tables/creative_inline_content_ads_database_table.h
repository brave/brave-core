/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_info.h"
#include "bat/ads/internal/database/database_table.h"
#include "bat/ads/internal/database/tables/campaigns_database_table.h"
#include "bat/ads/internal/database/tables/creative_ads_database_table.h"
#include "bat/ads/internal/database/tables/dayparts_database_table.h"
#include "bat/ads/internal/database/tables/geo_targets_database_table.h"
#include "bat/ads/internal/database/tables/segments_database_table.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

namespace ads {

using GetCreativeInlineContentAdCallback =
    std::function<void(const Result result,
                       const std::string& creative_instance_id,
                       const CreativeInlineContentAdInfo& ad)>;

using GetCreativeInlineContentAdsCallback =
    std::function<void(const Result result,
                       const SegmentList& segments,
                       const CreativeInlineContentAdList& ads)>;

namespace database {
namespace table {

class CreativeInlineContentAds : public Table {
 public:
  CreativeInlineContentAds();

  ~CreativeInlineContentAds() override;

  void Save(const CreativeInlineContentAdList& creative_inline_content_ads,
            ResultCallback callback);

  void Delete(ResultCallback callback);

  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetCreativeInlineContentAdCallback callback);

  void GetForSegments(const SegmentList& segments,
                      const std::string& dimensions,
                      GetCreativeInlineContentAdsCallback callback);

  void GetAll(GetCreativeInlineContentAdsCallback callback);

  void set_batch_size(const int batch_size);

  std::string get_table_name() const override;

  void Migrate(DBTransaction* transaction, const int to_version) override;

 private:
  void InsertOrUpdate(
      DBTransaction* transaction,
      const CreativeInlineContentAdList& creative__inline_content_ads);

  int BindParameters(
      DBCommand* command,
      const CreativeInlineContentAdList& creative__inline_content_ads);

  std::string BuildInsertOrUpdateQuery(
      DBCommand* command,
      const CreativeInlineContentAdList& creative__inline_content_ads);

  void OnGetForCreativeInstanceId(DBCommandResponsePtr response,
                                  const std::string& creative_instance_id,
                                  GetCreativeInlineContentAdCallback callback);

  void OnGetForSegments(DBCommandResponsePtr response,
                        const SegmentList& segments,
                        GetCreativeInlineContentAdsCallback callback);

  void OnGetAll(DBCommandResponsePtr response,
                GetCreativeInlineContentAdsCallback callback);

  CreativeInlineContentAdInfo GetFromRecord(DBRecord* record) const;

  void CreateTableV15(DBTransaction* transaction);
  void MigrateToV15(DBTransaction* transaction);

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

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_INLINE_CONTENT_ADS_DATABASE_TABLE_H_

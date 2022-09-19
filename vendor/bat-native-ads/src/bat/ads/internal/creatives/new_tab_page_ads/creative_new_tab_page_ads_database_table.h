/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_ADS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_ADS_DATABASE_TABLE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/check_op.h"
#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/internal/segments/segment_alias.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::database::table {

class Campaigns;
class CreativeAds;
class CreativeNewTabPageAdWallpapers;
class Dayparts;
class Deposits;
class GeoTargets;
class Segments;

using GetCreativeNewTabPageAdCallback =
    std::function<void(const bool,
                       const std::string& creative_instance_id,
                       const CreativeNewTabPageAdInfo&)>;

using GetCreativeNewTabPageAdsCallback =
    std::function<void(const bool,
                       const std::vector<std::string>&,
                       const CreativeNewTabPageAdList&)>;

class CreativeNewTabPageAds final : public TableInterface {
 public:
  CreativeNewTabPageAds();

  CreativeNewTabPageAds(const CreativeNewTabPageAds&) = delete;
  CreativeNewTabPageAds& operator=(const CreativeNewTabPageAds&) = delete;

  ~CreativeNewTabPageAds() override;

  void Save(const CreativeNewTabPageAdList& creative_ads,
            ResultCallback callback);

  void Delete(ResultCallback callback) const;

  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetCreativeNewTabPageAdCallback callback);

  void GetForSegments(const SegmentList& segments,
                      GetCreativeNewTabPageAdsCallback callback);

  void GetAll(GetCreativeNewTabPageAdsCallback callback);

  void SetBatchSize(const int batch_size) {
    DCHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const CreativeNewTabPageAdList& creative_ads);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommandInfo* command,
      const CreativeNewTabPageAdList& creative_ads) const;

  void OnGetForCreativeInstanceId(const std::string& creative_instance_id,
                                  GetCreativeNewTabPageAdCallback callback,
                                  mojom::DBCommandResponseInfoPtr response);

  void OnGetForSegments(const SegmentList& segments,
                        GetCreativeNewTabPageAdsCallback callback,
                        mojom::DBCommandResponseInfoPtr response);

  void OnGetAll(GetCreativeNewTabPageAdsCallback callback,
                mojom::DBCommandResponseInfoPtr response);

  void MigrateToV24(mojom::DBTransactionInfo* transaction);

  int batch_size_;

  std::unique_ptr<Campaigns> campaigns_database_table_;
  std::unique_ptr<CreativeAds> creative_ads_database_table_;
  std::unique_ptr<CreativeNewTabPageAdWallpapers>
      creative_new_tab_page_ad_wallpapers_database_table_;
  std::unique_ptr<Dayparts> dayparts_database_table_;
  std::unique_ptr<Deposits> deposits_database_table_;
  std::unique_ptr<GeoTargets> geo_targets_database_table_;
  std::unique_ptr<Segments> segments_database_table_;
};

}  // namespace ads::database::table

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_ADS_DATABASE_TABLE_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_CREATIVE_NOTIFICATION_ADS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_CREATIVE_NOTIFICATION_ADS_DATABASE_TABLE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/check_op.h"
#include "bat/ads/ads_client_aliases.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info_aliases.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/internal/segments/segments_aliases.h"
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

using GetCreativeNotificationAdsCallback =
    std::function<void(const bool,
                       const std::vector<std::string>&,
                       const CreativeNotificationAdList&)>;

class CreativeNotificationAds final : public TableInterface {
 public:
  CreativeNotificationAds();
  ~CreativeNotificationAds() override;
  CreativeNotificationAds(const CreativeNotificationAds&) = delete;
  CreativeNotificationAds& operator=(const CreativeNotificationAds&) = delete;

  void Save(const CreativeNotificationAdList& creative_notification_ads,
            ResultCallback callback);

  void Delete(ResultCallback callback);

  void GetForSegments(const SegmentList& segments,
                      GetCreativeNotificationAdsCallback callback);

  void GetAll(GetCreativeNotificationAdsCallback callback);

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
      const CreativeNotificationAdList& creative_notification_ads);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommand* command,
      const CreativeNotificationAdList& creative_notification_ads);

  void OnGetForSegments(mojom::DBCommandResponsePtr response,
                        const SegmentList& segments,
                        GetCreativeNotificationAdsCallback callback);

  void OnGetAll(mojom::DBCommandResponsePtr response,
                GetCreativeNotificationAdsCallback callback);

  void MigrateToV24(mojom::DBTransaction* transaction);

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

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NOTIFICATION_ADS_CREATIVE_NOTIFICATION_ADS_DATABASE_TABLE_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_AD_NOTIFICATIONS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_AD_NOTIFICATIONS_DATABASE_TABLE_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/database/database_table.h"
#include "bat/ads/internal/database/tables/campaigns_database_table.h"
#include "bat/ads/internal/database/tables/creative_ads_database_table.h"
#include "bat/ads/internal/database/tables/dayparts_database_table.h"
#include "bat/ads/internal/database/tables/geo_targets_database_table.h"
#include "bat/ads/internal/database/tables/segments_database_table.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

namespace ads {

using GetCreativeAdNotificationsCallback =
    std::function<void(const Result,
                       const std::vector<std::string>&,
                       const CreativeAdNotificationList&)>;

namespace database {
namespace table {

class CreativeAdNotifications : public Table {
 public:
  CreativeAdNotifications();

  ~CreativeAdNotifications() override;

  void Save(const CreativeAdNotificationList& creative_ad_notifications,
            ResultCallback callback);

  void Delete(ResultCallback callback);

  void GetForSegments(const SegmentList& segments,
                      GetCreativeAdNotificationsCallback callback);

  void GetAll(GetCreativeAdNotificationsCallback callback);

  void set_batch_size(const int batch_size);

  std::string get_table_name() const override;

  void Migrate(DBTransaction* transaction, const int to_version) override;

 private:
  void InsertOrUpdate(
      DBTransaction* transaction,
      const CreativeAdNotificationList& creative_ad_notifications);

  int BindParameters(
      DBCommand* command,
      const CreativeAdNotificationList& creative_ad_notifications);

  std::string BuildInsertOrUpdateQuery(
      DBCommand* command,
      const CreativeAdNotificationList& creative_ad_notifications);

  void OnGetForSegments(DBCommandResponsePtr response,
                        const SegmentList& segments,
                        GetCreativeAdNotificationsCallback callback);

  void OnGetAll(DBCommandResponsePtr response,
                GetCreativeAdNotificationsCallback callback);

  CreativeAdNotificationInfo GetFromRecord(DBRecord* record) const;

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

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CREATIVE_AD_NOTIFICATIONS_DATABASE_TABLE_H_

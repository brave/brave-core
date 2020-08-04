/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_DATABASE_CREATIVE_AD_NOTIFICATIONS_DATABASE_TABLE_H_
#define BAT_ADS_INTERNAL_DATABASE_CREATIVE_AD_NOTIFICATIONS_DATABASE_TABLE_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/classification/page_classifier/page_classifier.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/database/database_table.h"
#include "bat/ads/internal/database/tables/categories_database_table.h"
#include "bat/ads/internal/database/tables/geo_targets_database_table.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

namespace ads {

using GetCreativeAdNotificationsCallback = std::function<void(const Result,
    const std::vector<std::string>&, const CreativeAdNotificationList&)>;

class AdsImpl;

namespace database {
namespace table {

class CreativeAdNotifications : public Table {
 public:
  explicit CreativeAdNotifications(
      AdsImpl* ads);

  ~CreativeAdNotifications() override;

  void Save(
      const CreativeAdNotificationList& creative_ad_notifications,
      ResultCallback callback);

  void GetCreativeAdNotifications(
      const classification::CategoryList& categories,
      GetCreativeAdNotificationsCallback callback);

  void GetAllCreativeAdNotifications(
      GetCreativeAdNotificationsCallback callback);

  void set_batch_size(
      const int batch_size);

  std::string get_table_name() const override;

  void Migrate(
      DBTransaction* transaction,
      const int to_version) override;

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

  void OnGetCreativeAdNotifications(
      DBCommandResponsePtr response,
      const classification::CategoryList& categories,
      GetCreativeAdNotificationsCallback callback);

  void OnGetAllCreativeAdNotifications(
      DBCommandResponsePtr response,
      GetCreativeAdNotificationsCallback callback);

  CreativeAdNotificationInfo GetCreativeAdNotificationFromRecord(
      DBRecord* record) const;

  void DeleteAllTables(
      DBTransaction* transaction) const;

  void CreateTableV1(
      DBTransaction* transaction);
  void MigrateToV1(
      DBTransaction* transaction);

  int batch_size_;

  AdsImpl* ads_;  // NOT OWNED

  std::unique_ptr<GeoTargets> geo_targets_database_table_;
  std::unique_ptr<Categories> categories_database_table_;
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_DATABASE_CREATIVE_AD_NOTIFICATIONS_DATABASE_TABLE_H_

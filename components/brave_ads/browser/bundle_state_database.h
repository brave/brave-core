/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BUNDLE_STATE_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BUNDLE_STATE_DATABASE_H_

#include <stddef.h>
#include <string>
#include <vector>
#include <memory>

#include "bat/ads/creative_ad_notification_info.h"
#include "bat/ads/creative_publisher_ad_info.h"
#include "bat/ads/ad_conversion_tracking_info.h"
#include "bat/ads/bundle_state.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "build/build_config.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"

namespace brave_ads {

class BundleStateDatabase {
 public:
  explicit BundleStateDatabase(
      const base::FilePath& db_path);
  ~BundleStateDatabase();

  // Call before Init() to set the error callback to be used for the
  // underlying database connection.
  void set_error_callback(
      const sql::Database::ErrorCallback& error_callback) {
    db_.set_error_callback(error_callback);
  }

  bool SaveBundleState(
      const ads::BundleState& bundle_state);

  bool GetCreativeAdNotifications(
      const std::vector<std::string>& categories,
      ads::CreativeAdNotifications* ads);
  bool GetCreativePublisherAds(
      const std::string& url,
      const std::vector<std::string>& categories,
      const std::vector<std::string>& sizes,
      ads::CreativePublisherAds* ads);
  bool GetAdConversions(
      const std::string& url,
      ads::AdConversions* ad_conversions);

  // Returns the current version of the publisher info database
  static int GetCurrentVersion();

  // Vacuums the database. This will cause sqlite to defragment and collect
  // unused space in the file. It can be VERY SLOW.
  void Vacuum();

  std::string GetDiagnosticInfo(
      int extended_error,
      sql::Statement* statement);

 private:
  bool Init();
  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  bool CreateCategoryTable();
  bool TruncateCategoryTable();
  bool InsertOrUpdateCategory(
      const std::string& category);

  bool CreateCreativeAdNotificationInfoTable();
  bool TruncateCreativeAdNotificationInfoTable();
  bool InsertOrUpdateCreativeAdNotificationInfo(
      const ads::CreativeAdNotificationInfo& info);
  bool CreateCreativeAdNotificationInfoCategoryTable();
  bool TruncateCreativeAdNotificationInfoCategoryTable();
  bool CreateCreativeAdNotificationInfoCategoryNameIndex();
  bool InsertOrUpdateCreativeAdNotificationInfoCategory(
      const ads::CreativeAdNotificationInfo& info,
      const std::string& category);

  bool CreateCreativePublisherAdInfoTable();
  bool TruncateCreativePublisherAdInfoTable();
  bool InsertOrUpdateCreativePublisherAdInfo(
      const ads::CreativePublisherAdInfo& info);

  bool CreateCreativePublisherAdInfoCategoryTable();
  bool TruncateCreativePublisherAdInfoCategoryTable();
  bool CreateCreativePublisherAdInfoCategoryNameIndex();
  bool InsertOrUpdateCreativePublisherAdInfoCategory(
      const ads::CreativePublisherAdInfo& info,
      const std::string& category);

  bool CreateAdConversionsTable();
  bool TruncateAdConversionsTable();
  bool InsertOrUpdateAdConversion(
      const ads::AdConversionTrackingInfo& ad_conversion);

  sql::Database& GetDB();
  sql::MetaTable& GetMetaTable();

  bool Migrate();
  bool MigrateV1toV2();
  bool MigrateV2toV3();
  bool MigrateV3toV4();
  sql::InitStatus EnsureCurrentVersion();

  sql::Database db_;
  sql::MetaTable meta_table_;
  const base::FilePath db_path_;
  bool initialized_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(BundleStateDatabase);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BUNDLE_STATE_DATABASE_H_

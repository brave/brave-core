/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BUNDLE_DATA_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BUNDLE_DATA_DATABASE_H_

#include <memory>
#include <stddef.h>

#include "bat/ads/ad_info.h"
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
  BundleStateDatabase(const base::FilePath& db_path);
  ~BundleStateDatabase();

  // Call before Init() to set the error callback to be used for the
  // underlying database connection.
  void set_error_callback(const sql::Database::ErrorCallback& error_callback) {
    db_.set_error_callback(error_callback);
  }

  bool SaveBundleState(const ads::BUNDLE_STATE& bundle_state);
  bool GetAdsForCategory(const std::string& category,
                         std::vector<ads::AdInfo>& ads);

  // Returns the current version of the publisher info database
  static int GetCurrentVersion();

  // Vacuums the database. This will cause sqlite to defragment and collect
  // unused space in the file. It can be VERY SLOW.
  void Vacuum();

  std::string GetDiagnosticInfo(int extended_error, sql::Statement* statement);

 private:
  bool Init();
  void OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  bool CreateCategoryTable();
  bool CreateAdInfoTable();
  bool CreateAdInfoCategoryTable();
  bool CreateAdInfoCategoryNameIndex();

  bool TruncateCategoryTable();
  bool TruncateAdInfoTable();
  bool TruncateAdInfoCategoryTable();

  bool InsertOrUpdateCategory(const std::string& category);
  bool InsertOrUpdateAdInfo(const ads::AdInfo& info);
  bool InsertOrUpdateAdInfoCategory(const ads::AdInfo& ad_info,
                                    const std::string& category);

  sql::Database& GetDB();
  sql::MetaTable& GetMetaTable();

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

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BUNDLE_DATA_DATABASE_H_

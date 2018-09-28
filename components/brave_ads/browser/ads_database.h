/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_USERMODEL_ADS_DATABASE_H_
#define BRAVE_BROWSER_BRAVE_USERMODEL_ADS_DATABASE_H_

#include <stdint.h>
#include <string>
#include <set>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/i18n/time_formatting.h"
#include "build/build_config.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "build/build_config.h"
#include "sql/init_status.h"

#include "ad.h"

namespace brave_ads {

class AdsDatabase {
 public:
  AdsDatabase(const base::FilePath& db_path);
  ~AdsDatabase();

  // Call before Init() to set the error callback to be used for the
  // underlying database connection.
  void set_error_callback(const sql::Database::ErrorCallback& error_callback) {
    db_.set_error_callback(error_callback);
  }

  bool PushToHistory(const usermodel::Ad& ad);

  bool AdsSeen(time_t timestamp, std::set<std::string> *set);

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
  bool CreateAdsHistoryTable();
  bool CreateHistoryIndex();

  sql::Database& GetDB();
  sql::MetaTable& GetMetaTable();

  sql::InitStatus EnsureCurrentVersion();

  sql::Database db_;
  sql::MetaTable meta_table_;
  const base::FilePath db_path_;
  bool initialized_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(AdsDatabase);
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_USERMODEL_ADS_DATABASE_H_
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_PUBLISHER_INFO_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_PUBLISHER_INFO_DATABASE_H_

#include <memory>
#include <stddef.h>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "bat/ledger/publisher_info.h"
#include "build/build_config.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"

namespace ledger {
struct MediaPublisherInfo;
}

namespace brave_rewards {

class PublisherInfoDatabase {
 public:
  PublisherInfoDatabase(const base::FilePath& db_path);
  ~PublisherInfoDatabase();

  // Call before Init() to set the error callback to be used for the
  // underlying database connection.
  void set_error_callback(const sql::Database::ErrorCallback& error_callback) {
    db_.set_error_callback(error_callback);
  }

  bool InsertOrUpdatePublisherInfo(const ledger::PublisherInfo& info);
  bool InsertOrUpdateMediaPublisherInfo(const ledger::MediaPublisherInfo& info);

  bool Find(int start,
            int limit,
            const ledger::PublisherInfoFilter& filter,
            ledger::PublisherInfoList* list);
  std::unique_ptr<ledger::MediaPublisherInfo> GetMediaPublisherInfo(
      const std::string& publisher_id);

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
  bool CreateContributionInfoTable();
  bool CreatePublisherInfoTable();
  bool CreateMediaPublisherInfoTable();
  bool CreateActivityInfoTable();
  bool CreateContributionInfoIndex();
  bool CreateActivityInfoIndex();

  sql::Database& GetDB();
  sql::MetaTable& GetMetaTable();

  sql::InitStatus EnsureCurrentVersion();

  sql::Database db_;
  sql::MetaTable meta_table_;
  const base::FilePath db_path_;
  bool initialized_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(PublisherInfoDatabase);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_PUBLISHER_INFO_DATABASE_H_

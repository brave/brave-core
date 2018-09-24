/*
#include <stdint.h>

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "bat/ledger/media_publisher_info.h"
#include "build/build_config.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

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

  bool InsertOrUpdateAdsDatabase(const ledger::PublisherInfo& info);

  bool Find(int start,
            int limit,
            const ledger::PublisherInfoFilter& filter,
            ledger::PublisherInfoList* list);
  std::unique_ptr<ledger::PublisherInfo> GetMediaPublisherInfo(
      const std::string& media_key);

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
  DISALLOW_COPY_AND_ASSIGN(AdsDatabase);
};

}  // namespace brave_rewards
*/
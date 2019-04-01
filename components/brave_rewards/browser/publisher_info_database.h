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
#include "bat/ledger/pending_contribution.h"
#include "brave/components/brave_rewards/browser/contribution_info.h"
#include "brave/components/brave_rewards/browser/pending_contribution.h"
#include "brave/components/brave_rewards/browser/recurring_donation.h"
#include "build/build_config.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"

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

  bool InsertContributionInfo(const brave_rewards::ContributionInfo& info);

  void GetOneTimeTips(ledger::PublisherInfoList* list,
                      ledger::ACTIVITY_MONTH month,
                      int year);

  bool InsertOrUpdatePublisherInfo(const ledger::PublisherInfo& info);

  std::unique_ptr<ledger::PublisherInfo> GetPublisherInfo(
     const std::string& media_key);

  std::unique_ptr<ledger::PublisherInfo> GetPanelPublisher(
     const ledger::ActivityInfoFilter& filter);

  bool RestorePublishers();

  int GetExcludedPublishersCount();

  bool InsertOrUpdateActivityInfo(const ledger::PublisherInfo& info);

  bool InsertOrUpdateActivityInfos(const ledger::PublisherInfoList& list);

  bool GetActivityList(int start,
                       int limit,
                       const ledger::ActivityInfoFilter& filter,
                       ledger::PublisherInfoList* list);

  bool InsertOrUpdateMediaPublisherInfo(const std::string& media_key,
                                        const std::string& publisher_id);

  std::unique_ptr<ledger::PublisherInfo> GetMediaPublisherInfo(
      const std::string& media_key);

  bool InsertOrUpdateRecurringDonation(
      const brave_rewards::RecurringDonation& info);

  void GetRecurringDonations(ledger::PublisherInfoList* list);

  bool RemoveRecurring(const std::string& publisher_key);
  bool InsertPendingContribution(const ledger::PendingContributionList& list);
  double GetReservedAmount();


  // Returns the current version of the publisher info database
  int GetCurrentVersion();

  void SetTestingCurrentVersion(int value);

  bool DeleteActivityInfo(const std::string& publisher_key,
                          uint64_t reconcile_stamp);

  // Vacuums the database. This will cause sqlite to defragment and collect
  // unused space in the file. It can be VERY SLOW.
  void Vacuum();

  std::string GetDiagnosticInfo(int extended_error, sql::Statement* statement);

  sql::Database& GetDB();

  bool Init();

  int GetTableVersionNumber();

 private:

  bool CreateContributionInfoTable();

  bool CreateContributionInfoIndex();

  bool CreatePublisherInfoTable();

  bool CreateActivityInfoTable();

  bool CreateActivityInfoIndex();

  bool CreateMediaPublisherInfoTable();

  bool CreateRecurringDonationTable();

  bool CreateRecurringDonationIndex();
  bool CreatePendingContributionsTable();
  bool CreatePendingContributionsIndex();

  void OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  sql::MetaTable& GetMetaTable();

  bool MigrateV1toV2();

  bool MigrateV2toV3();

  bool MigrateV3toV4();

  bool MigrateV4toV5();

  bool MigrateV5toV6();

  bool Migrate(int version);

  sql::InitStatus EnsureCurrentVersion();

  sql::Database db_;
  sql::MetaTable meta_table_;
  const base::FilePath db_path_;
  bool initialized_;
  int testing_current_version_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(PublisherInfoDatabase);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_PUBLISHER_INFO_DATABASE_H_

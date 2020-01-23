/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_PUBLISHER_INFO_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_PUBLISHER_INFO_DATABASE_H_

#include <stddef.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/sequence_checker.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_contribution_info.h"
#include "brave/components/brave_rewards/browser/database/database_contribution_queue.h"
#include "brave/components/brave_rewards/browser/database/database_media_publisher_info.h"
#include "brave/components/brave_rewards/browser/database/database_pending_contribution.h"
#include "brave/components/brave_rewards/browser/database/database_promotion.h"
#include "brave/components/brave_rewards/browser/database/database_recurring_tip.h"
#include "brave/components/brave_rewards/browser/database/database_unblinded_token.h"
#include "brave/components/brave_rewards/browser/pending_contribution.h"
#include "build/build_config.h"
#include "sql/database.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"

namespace brave_rewards {

class PublisherInfoDatabase {
 public:
  PublisherInfoDatabase(
      const base::FilePath& db_path,
      const int testing_current_version = -1);
  ~PublisherInfoDatabase();

  // Call before Init() to set the error callback to be used for the
  // underlying database connection.
  void set_error_callback(const sql::Database::ErrorCallback& error_callback) {
    db_.set_error_callback(error_callback);
  }

  bool InsertOrUpdateContributionInfo(ledger::ContributionInfoPtr info);

  void GetOneTimeTips(
      ledger::PublisherInfoList* list,
      const ledger::ActivityMonth month,
      const int year);

  bool InsertOrUpdateMediaPublisherInfo(
      const std::string& media_key,
      const std::string& publisher_key);

  ledger::PublisherInfoPtr GetMediaPublisherInfo(
      const std::string& media_key);

  bool InsertOrUpdateRecurringTip(ledger::RecurringTipPtr info);

  void GetRecurringTips(ledger::PublisherInfoList* list);

  bool RemoveRecurringTip(const std::string& publisher_key);

  bool InsertPendingContribution(ledger::PendingContributionList list);

  double GetReservedAmount();

  void GetPendingContributions(
      ledger::PendingContributionInfoList* list);

  bool RemovePendingContributions(const uint64_t id);

  bool RemoveAllPendingContributions();

  bool InsertOrUpdateContributionQueue(ledger::ContributionQueuePtr info);

  ledger::ContributionQueuePtr GetFirstContributionQueue();

  bool DeleteContributionQueue(const uint64_t id);

  bool InsertOrUpdatePromotion(ledger::PromotionPtr info);

  ledger::PromotionPtr GetPromotion(const std::string& id);

  ledger::PromotionMap GetAllPromotions();

  bool DeletePromotionList(const std::vector<std::string>& id_list);

  bool SaveUnblindedTokenList(ledger::UnblindedTokenList list);

  ledger::UnblindedTokenList GetAllUnblindedTokens();

  bool DeleteUnblindedTokens(const std::vector<std::string>& id_list);

  bool DeleteUnblindedTokensForPromotion(
      const std::string& promotion_id);

  void RecordP3AStats(bool auto_contributions_on);

  // Returns the current version of the publisher info database
  int GetCurrentVersion();

  void GetTransactionReport(
      ledger::TransactionReportInfoList* list,
      const ledger::ActivityMonth month,
      const int year);

  void GetContributionReport(
      ledger::ContributionReportInfoList* list,
      const ledger::ActivityMonth month,
      const int year);

  void GetIncompleteContributions(
      ledger::ContributionInfoList* list);

  ledger::ContributionInfoPtr GetContributionInfo(
      const std::string& contribution_id);

  bool UpdateContributionInfoStepAndCount(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      const int32_t retry_count);

  bool UpdateContributionInfoContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key);

  // Vacuums the database. This will cause sqlite to defragment and collect
  // unused space in the file. It can be VERY SLOW.
  void Vacuum();

  std::string GetDiagnosticInfo(int extended_error, sql::Statement* statement);

  sql::Database& GetDB();

  bool Init();

  int GetTableVersionNumber();

  std::string GetSchema();

 private:
  bool IsInitialized();

  void OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  bool InitMetaTable(const int version);

  sql::MetaTable& GetMetaTable();

  bool MigrateV0toV1();

  bool MigrateV1toV2();

  bool MigrateV2toV3();

  bool MigrateV3toV4();

  bool MigrateV4toV5();

  bool MigrateV5toV6();

  bool MigrateV6toV7();

  bool MigrateV7toV8();

  bool MigrateV8toV9();

  bool MigrateV9toV10();

  bool MigrateV10toV11();

  bool MigrateV11toV12();

  bool MigrateV12toV13();

  bool MigrateV13toV14();

  bool MigrateV14toV15();

  bool Migrate(int version);

  sql::InitStatus EnsureCurrentVersion(const int table_version);

  sql::Database db_;
  sql::MetaTable meta_table_;
  const base::FilePath db_path_;
  bool initialized_;
  int testing_current_version_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;
  std::unique_ptr<DatabaseContributionQueue> contribution_queue_;
  std::unique_ptr<DatabasePromotion> promotion_;
  std::unique_ptr<DatabaseUnblindedToken> unblinded_token_;
  std::unique_ptr<DatabaseContributionInfo> contribution_info_;
  std::unique_ptr<DatabasePendingContribution> pending_contribution_;
  std::unique_ptr<DatabaseMediaPublisherInfo> media_publisher_info_;
  std::unique_ptr<DatabaseRecurringTip> recurring_tip_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(PublisherInfoDatabase);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_PUBLISHER_INFO_DATABASE_H_

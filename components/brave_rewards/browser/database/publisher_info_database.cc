/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/database/publisher_info_database.h"
#include "brave/components/brave_rewards/browser/database/database_util.h"

namespace brave_rewards {

namespace {

const int kCurrentVersionNumber = 13;
const int kCompatibleVersionNumber = 1;

}  // namespace

PublisherInfoDatabase::PublisherInfoDatabase(
    const base::FilePath& db_path,
    const int testing_current_version) :
    db_path_(db_path),
    initialized_(false),
    testing_current_version_(testing_current_version) {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  server_publisher_info_ =
      std::make_unique<DatabaseServerPublisherInfo>(GetCurrentVersion());

  contribution_queue_ =
      std::make_unique<DatabaseContributionQueue>(GetCurrentVersion());

  promotion_ =
      std::make_unique<DatabasePromotion>(GetCurrentVersion());

  unblinded_token_ =
      std::make_unique<DatabaseUnblindedToken>(GetCurrentVersion());

  contribution_info_ =
      std::make_unique<DatabaseContributionInfo>(GetCurrentVersion());

  pending_contribution_ =
      std::make_unique<DatabasePendingContribution>(GetCurrentVersion());

  media_publisher_info_ =
      std::make_unique<DatabaseMediaPublisherInfo>(GetCurrentVersion());

  recurring_tip_ =
      std::make_unique<DatabaseRecurringTip>(GetCurrentVersion());

  publisher_info_ =
      std::make_unique<DatabasePublisherInfo>(GetCurrentVersion());

  activity_info_ =
      std::make_unique<DatabaseActivityInfo>(GetCurrentVersion());
}

PublisherInfoDatabase::~PublisherInfoDatabase() {
}

bool PublisherInfoDatabase::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (initialized_) {
    return true;
  }

  if (!db_.Open(db_path_)) {
    return false;
  }

  // TODO(brave): Add error delegate
  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    return false;
  }

  if (!meta_table_.Init(&db_, GetCurrentVersion(), kCompatibleVersionNumber)) {
    return false;
  }

  if (!publisher_info_->Init(&GetDB())) {
    return false;
  }

  if (!server_publisher_info_->Init(&GetDB())) {
    return false;
  }

  if (!contribution_queue_->Init(&GetDB())) {
    return false;
  }

  if (!promotion_->Init(&GetDB())) {
    return false;
  }

  if (!unblinded_token_->Init(&GetDB())) {
    return false;
  }

  if (!contribution_info_->Init(&GetDB())) {
    return false;
  }

  if (!pending_contribution_->Init(&GetDB())) {
    return false;
  }

  if (!media_publisher_info_->Init(&GetDB())) {
    return false;
  }

  if (!recurring_tip_->Init(&GetDB())) {
    return false;
  }

  if (!activity_info_->Init(&GetDB())) {
    return false;
  }

  // Version check.
  sql::InitStatus version_status = EnsureCurrentVersion();
  if (version_status != sql::INIT_OK) {
    return version_status;
  }

  if (!committer.Commit()) {
    return false;
  }

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&PublisherInfoDatabase::OnMemoryPressure,
      base::Unretained(this))));

  initialized_ = true;
  return initialized_;
}

/**
 *
 * CONTRIBUTION INFO
 *
 */
bool PublisherInfoDatabase::InsertOrUpdateContributionInfo(
    ledger::ContributionInfoPtr info) {
  if (!IsInitialized()) {
    return false;
  }

  return contribution_info_->InsertOrUpdate(&GetDB(), std::move(info));
}

void PublisherInfoDatabase::GetOneTimeTips(
    ledger::PublisherInfoList* list,
    const ledger::ActivityMonth month,
    const int year) {
  if (!IsInitialized()) {
    return;
  }

  contribution_info_->GetOneTimeTips(&GetDB(), list, month, year);
}

/**
 *
 * PUBLISHER INFO
 *
 */

bool PublisherInfoDatabase::InsertOrUpdatePublisherInfo(
    ledger::PublisherInfoPtr info) {
  if (!IsInitialized()) {
    return false;
  }

  return publisher_info_->InsertOrUpdate(&GetDB(), std::move(info));
}

ledger::PublisherInfoPtr
PublisherInfoDatabase::GetPublisherInfo(const std::string& publisher_key) {
  if (!IsInitialized()) {
    return nullptr;
  }

  return publisher_info_->GetRecord(&GetDB(), publisher_key);
}

ledger::PublisherInfoPtr
PublisherInfoDatabase::GetPanelPublisher(
    ledger::ActivityInfoFilterPtr filter) {
  if (!IsInitialized()) {
    return nullptr;
  }

  return publisher_info_->GetPanelRecord(&GetDB(), std::move(filter));
}

bool PublisherInfoDatabase::RestorePublishers() {
  if (!IsInitialized()) {
    return false;
  }

  return publisher_info_->RestorePublishers(&GetDB());
}

bool PublisherInfoDatabase::GetExcludedList(
    ledger::PublisherInfoList* list) {
  if (!IsInitialized()) {
    return false;
  }

  return publisher_info_->GetExcludedList(&GetDB(), list);
}

/**
 *
 * ACTIVITY INFO
 *
 */
bool PublisherInfoDatabase::InsertOrUpdateActivityInfo(
    ledger::PublisherInfoPtr info) {
  if (!IsInitialized()) {
    return false;
  }

  if (!InsertOrUpdatePublisherInfo(info->Clone())) {
    return false;
  }

  return activity_info_->InsertOrUpdate(&GetDB(), info->Clone());
}

bool PublisherInfoDatabase::InsertOrUpdateActivityInfos(
    ledger::PublisherInfoList list) {
  if (!IsInitialized()) {
    return false;
  }

  if (list.size() == 0) {
    return true;
  }

  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  for (auto& info : list) {
    if (!InsertOrUpdateActivityInfo(std::move(info))) {
      transaction.Rollback();
      return false;
    }
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::GetActivityList(
    int start,
    int limit,
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoList* list) {
  if (!IsInitialized()) {
    return false;
  }

  return activity_info_->GetRecordsList(
      &GetDB(),
      start,
      limit,
      std::move(filter),
      list);
}

bool PublisherInfoDatabase::DeleteActivityInfo(
    const std::string& publisher_key,
    uint64_t reconcile_stamp) {
  if (!IsInitialized()) {
    return false;
  }

  return activity_info_->DeleteRecord(&GetDB(), publisher_key, reconcile_stamp);
}

/**
 *
 * MEDIA PUBLISHER INFO
 *
 */
bool PublisherInfoDatabase::InsertOrUpdateMediaPublisherInfo(
    const std::string& media_key,
    const std::string& publisher_key) {
  if (!IsInitialized()) {
    return false;
  }

  return media_publisher_info_->InsertOrUpdate(
      &GetDB(),
      media_key,
      publisher_key);
}

ledger::PublisherInfoPtr
PublisherInfoDatabase::GetMediaPublisherInfo(const std::string& media_key) {
  if (!IsInitialized()) {
    return nullptr;
  }

  return media_publisher_info_->GetRecord(&GetDB(), media_key);
}

/**
 *
 * RECURRING TIPS
 *
 */
bool PublisherInfoDatabase::InsertOrUpdateRecurringTip(
    ledger::RecurringTipPtr info) {
  if (!IsInitialized()) {
    return false;
  }

  return recurring_tip_->InsertOrUpdate(&GetDB(), std::move(info));
}

void PublisherInfoDatabase::GetRecurringTips(
    ledger::PublisherInfoList* list) {
  if (!IsInitialized()) {
    return;
  }

  recurring_tip_->GetAllRecords(&GetDB(), list);
}

bool PublisherInfoDatabase::RemoveRecurringTip(
    const std::string& publisher_key) {
  if (!IsInitialized()) {
    return false;
  }

  return recurring_tip_->DeleteRecord(&GetDB(), publisher_key);
}

/**
 *
 * PENDING CONTRIBUTION
 *
 */
bool PublisherInfoDatabase::InsertPendingContribution(
    ledger::PendingContributionList list) {
  if (!IsInitialized()) {
    return false;
  }

  return pending_contribution_->InsertOrUpdate(&GetDB(), std::move(list));
}

double PublisherInfoDatabase::GetReservedAmount() {
  if (!IsInitialized()) {
    return 0.0;
  }

  return pending_contribution_->GetReservedAmount(&GetDB());
}

void PublisherInfoDatabase::GetPendingContributions(
    ledger::PendingContributionInfoList* list) {
  if (!IsInitialized()) {
    return;
  }

  return pending_contribution_->GetAllRecords(&GetDB(), list);
}

bool PublisherInfoDatabase::RemovePendingContributions(const uint64_t id) {
  if (!IsInitialized()) {
    return false;
  }

  return pending_contribution_->DeleteRecord(&GetDB(), id);
}

bool PublisherInfoDatabase::RemoveAllPendingContributions() {
  if (!IsInitialized()) {
    return false;
  }

  return pending_contribution_->DeleteAllRecords(&GetDB());
}

/**
 *
 * SERVER PUBLISHER
 *
 */
bool PublisherInfoDatabase::ClearAndInsertServerPublisherList(
    const ledger::ServerPublisherInfoList& list) {
  if (!IsInitialized()) {
    return false;
  }

  return server_publisher_info_->ClearAndInsertList(&GetDB(), list);
}

ledger::ServerPublisherInfoPtr PublisherInfoDatabase::GetServerPublisherInfo(
    const std::string& publisher_key) {
  if (!IsInitialized()) {
    return nullptr;
  }

  return server_publisher_info_->GetRecord(&GetDB(), publisher_key);
}

/**
 *
 * CONTRIBUTION QUEUE
 *
 */
bool PublisherInfoDatabase::InsertOrUpdateContributionQueue(
    ledger::ContributionQueuePtr info) {
  if (!IsInitialized()) {
    return false;
  }

  return contribution_queue_->InsertOrUpdate(&GetDB(), std::move(info));
}

ledger::ContributionQueuePtr
PublisherInfoDatabase::GetFirstContributionQueue() {
  if (!IsInitialized()) {
    return nullptr;
  }

  return contribution_queue_->GetFirstRecord(&GetDB());
}

bool PublisherInfoDatabase::DeleteContributionQueue(const uint64_t id) {
  if (!IsInitialized()) {
    return false;
  }

  return contribution_queue_->DeleteRecord(&GetDB(), id);
}

/**
 *
 * PROMOTION
 *
 */
bool PublisherInfoDatabase::InsertOrUpdatePromotion(
    ledger::PromotionPtr info) {
  if (!IsInitialized()) {
    return false;
  }

  return promotion_->InsertOrUpdate(&GetDB(), std::move(info));
}

ledger::PromotionPtr
PublisherInfoDatabase::GetPromotion(const std::string& id) {
  if (!IsInitialized()) {
    return nullptr;
  }

  return promotion_->GetRecord(&GetDB(), id);
}

ledger::PromotionMap PublisherInfoDatabase::GetAllPromotions() {
  if (!IsInitialized()) {
    return {};
  }

  return promotion_->GetAllRecords(&GetDB());
}

/**
 *
 * UNBLINDED TOKEN
 *
 */
bool PublisherInfoDatabase::InsertOrUpdateUnblindedToken(
    ledger::UnblindedTokenPtr info) {
  if (!IsInitialized()) {
    return false;
  }

  return unblinded_token_->InsertOrUpdate(&GetDB(), std::move(info));
}

ledger::UnblindedTokenList
PublisherInfoDatabase::GetAllUnblindedTokens()  {
  if (!IsInitialized()) {
    ledger::UnblindedTokenList empty_list;
    return empty_list;
  }

  return unblinded_token_->GetAllRecords(&GetDB());
}

bool PublisherInfoDatabase::DeleteUnblindedTokens(
    const std::vector<std::string>& id_list) {
  if (!IsInitialized()) {
    return false;
  }

  return unblinded_token_->DeleteRecords(&GetDB(), id_list);
}

bool PublisherInfoDatabase::DeleteUnblindedTokensForPromotion(
    const std::string& promotion_id) {
  if (!IsInitialized()) {
    return false;
  }

  return DatabaseUnblindedToken::DeleteRecordsForPromotion(
      &GetDB(),
      promotion_id);
}

// Other -------------------------------------------------------------------

bool PublisherInfoDatabase::IsInitialized() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool initialized = Init();
  DCHECK(initialized);
  return initialized;
}

void PublisherInfoDatabase::RecordP3AStats(bool auto_contributions_on) {
  if (!initialized_) {
    return;
  }

  sql::Statement sql(
      db_.GetCachedStatement(SQL_FROM_HERE,
                             "SELECT type, COUNT(*) FROM contribution_info "
                             "GROUP BY 1"));
  int auto_contributions = 0;
  int tips = 0;
  int queued_recurring = 0;

  while (sql.Step()) {
    const int count = sql.ColumnInt(1);
    switch (sql.ColumnInt(0)) {
    case static_cast<int>(ledger::RewardsType::AUTO_CONTRIBUTE):
      auto_contributions = count;
      break;
    case static_cast<int>(ledger::RewardsType::ONE_TIME_TIP):
      tips += count;
      break;
    case static_cast<int>(ledger::RewardsType::RECURRING_TIP):
      queued_recurring = count;
      break;
    default:
      NOTREACHED();
    }
  }

  if (queued_recurring == 0) {
    // Check for queued recurring donations.
    sql::Statement sql(
        db_.GetUniqueStatement("SELECT COUNT(*) FROM recurring_donation"));
    if (sql.Step()) {
      queued_recurring = sql.ColumnInt(0);
    }
  }

  const auto auto_contributions_state = auto_contributions_on ?
        AutoContributionsP3AState::kAutoContributeOn :
        AutoContributionsP3AState::kWalletCreatedAutoContributeOff;
  RecordAutoContributionsState(auto_contributions_state, auto_contributions);
  RecordTipsState(true, tips, queued_recurring);
}

int PublisherInfoDatabase::GetCurrentVersion() {
  if (testing_current_version_ != -1) {
    return testing_current_version_;
  }

  return kCurrentVersionNumber;
}

void PublisherInfoDatabase::Vacuum() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!initialized_)
    return;

  DCHECK_EQ(0, db_.transaction_nesting()) <<
      "Can not have a transaction when vacuuming.";
  ignore_result(db_.Execute("VACUUM"));
}

void PublisherInfoDatabase::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

std::string PublisherInfoDatabase::GetDiagnosticInfo(int extended_error,
                                               sql::Statement* statement) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(initialized_);
  return db_.GetDiagnosticInfo(extended_error, statement);
}

sql::Database& PublisherInfoDatabase::GetDB() {
  return db_;
}

sql::MetaTable& PublisherInfoDatabase::GetMetaTable() {
  return meta_table_;
}

int PublisherInfoDatabase::GetTableVersionNumber() {
  return meta_table_.GetVersionNumber();
}

std::string PublisherInfoDatabase::GetSchema() {
  return db_.GetSchema();
}

// Migration -------------------------------------------------------------------

bool PublisherInfoDatabase::MigrateV1toV2() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!activity_info_->Migrate(&GetDB(), 2)) {
    return false;
  }

  if (!contribution_info_->Migrate(&GetDB(), 2)) {
    return false;
  }

  if (!recurring_tip_->Migrate(&GetDB(), 2)) {
    return false;
  }

  return true;
}

bool PublisherInfoDatabase::MigrateV2toV3() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!pending_contribution_->Migrate(&GetDB(), 3)) {
    return false;
  }

  return true;
}

bool PublisherInfoDatabase::MigrateV3toV4() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!activity_info_->Migrate(&GetDB(), 4)) {
    return false;
  }

  return true;
}

bool PublisherInfoDatabase::MigrateV4toV5() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!activity_info_->Migrate(&GetDB(), 5)) {
    return false;
  }

  return true;
}

bool PublisherInfoDatabase::MigrateV5toV6() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!activity_info_->Migrate(&GetDB(), 6)) {
    return false;
  }

  return true;
}

bool PublisherInfoDatabase::MigrateV6toV7() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  if (!publisher_info_->Migrate(&GetDB(), 7)) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV7toV8() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  if (!contribution_info_->Migrate(&GetDB(), 8)) {
    return false;
  }

  if (!pending_contribution_->Migrate(&GetDB(), 8)) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV8toV9() {
  // no need to call any script as database has min version
  // contribution queue tables
  return true;
}

bool PublisherInfoDatabase::MigrateV9toV10() {
  // no need to call any script as database has min version for
  // promotion and unblinded token tables
  return true;
}

bool PublisherInfoDatabase::MigrateV10toV11() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  if (!contribution_info_->Migrate(&GetDB(), 11)) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV11toV12() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  if (!pending_contribution_->Migrate(&GetDB(), 12)) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV12toV13() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  if (!promotion_->Migrate(&GetDB(), 13)) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::Migrate(int version) {
  switch (version) {
    case 2: {
      return MigrateV1toV2();
    }
    case 3: {
      return MigrateV2toV3();
    }
    case 4: {
      return MigrateV3toV4();
    }
    case 5: {
      return MigrateV4toV5();
    }
    case 6: {
      return MigrateV5toV6();
    }
    case 7: {
      return MigrateV6toV7();
    }
    case 8: {
      return MigrateV7toV8();
    }
    case 9: {
      return MigrateV8toV9();
    }
    case 10: {
      return MigrateV9toV10();
    }
    case 11: {
      return MigrateV10toV11();
    }
    case 12: {
      return MigrateV11toV12();
    }
    case 13: {
      return MigrateV12toV13();
    }
    default:
      NOTREACHED();
      return false;
  }
}

sql::InitStatus PublisherInfoDatabase::EnsureCurrentVersion() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // We can't read databases newer than we were designed for.
  if (meta_table_.GetCompatibleVersionNumber() > GetCurrentVersion()) {
    LOG(WARNING) << "Publisher info database is too new.";
    return sql::INIT_TOO_NEW;
  }

  const int old_version = GetTableVersionNumber();
  const int current_version = GetCurrentVersion();
  const int start_version = old_version + 1;

  int migrated_version = old_version;
  for (auto i = start_version; i <= current_version; i++) {
    if (!Migrate(i)) {
      LOG(ERROR) << "DB: Error with MigrateV" << (i - 1) << "toV" << i;
      break;
    }

    migrated_version = i;
  }

  meta_table_.SetVersionNumber(migrated_version);
  return sql::INIT_OK;
}

}  // namespace brave_rewards

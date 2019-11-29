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

const int kCurrentVersionNumber = 11;
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

  if (!CreateActivityInfoTable()) {
    return false;
  }

  CreateActivityInfoIndex();

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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return contribution_info_->InsertOrUpdate(&GetDB(), std::move(info));
}

void PublisherInfoDatabase::GetOneTimeTips(
    ledger::PublisherInfoList* list,
    const ledger::ActivityMonth month,
    const int year) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return publisher_info_->InsertOrUpdate(&GetDB(), std::move(info));
}

ledger::PublisherInfoPtr
PublisherInfoDatabase::GetPublisherInfo(const std::string& publisher_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return nullptr;
  }

  return publisher_info_->GetRecord(&GetDB(), publisher_key);
}

ledger::PublisherInfoPtr
PublisherInfoDatabase::GetPanelPublisher(
    ledger::ActivityInfoFilterPtr filter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return nullptr;
  }

  return publisher_info_->GetPanelRecord(&GetDB(), std::move(filter));
}

bool PublisherInfoDatabase::RestorePublishers() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return publisher_info_->RestorePublishers(&GetDB());
}

bool PublisherInfoDatabase::GetExcludedList(
    ledger::PublisherInfoList* list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return publisher_info_->GetExcludedList(&GetDB(), list);
}

/**
 *
 * ACTIVITY INFO
 *
 */
bool PublisherInfoDatabase::CreateActivityInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "activity_info";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "publisher_id LONGVARCHAR NOT NULL,"
      "duration INTEGER DEFAULT 0 NOT NULL,"
      "visits INTEGER DEFAULT 0 NOT NULL,"
      "score DOUBLE DEFAULT 0 NOT NULL,"
      "percent INTEGER DEFAULT 0 NOT NULL,"
      "weight DOUBLE DEFAULT 0 NOT NULL,"
      "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
      "CONSTRAINT activity_unique "
      "UNIQUE (publisher_id, reconcile_stamp) "
      "CONSTRAINT fk_activity_info_publisher_id"
      "    FOREIGN KEY (publisher_id)"
      "    REFERENCES publisher_info (publisher_id)"
      "    ON DELETE CASCADE)");

  return GetDB().Execute(sql.c_str());
}

bool PublisherInfoDatabase::CreateActivityInfoIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS activity_info_publisher_id_index "
      "ON activity_info (publisher_id)");
}

bool PublisherInfoDatabase::InsertOrUpdateActivityInfo(
    ledger::PublisherInfoPtr info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  if (!InsertOrUpdatePublisherInfo(info->Clone())) {
    return false;
  }

  sql::Statement activity_info_insert(
    GetDB().GetCachedStatement(SQL_FROM_HERE,
        "INSERT OR REPLACE INTO activity_info "
        "(publisher_id, duration, score, percent, "
        "weight, reconcile_stamp, visits) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));

  activity_info_insert.BindString(0, info->id);
  activity_info_insert.BindInt64(1, static_cast<int>(info->duration));
  activity_info_insert.BindDouble(2, info->score);
  activity_info_insert.BindInt64(3, static_cast<int>(info->percent));
  activity_info_insert.BindDouble(4, info->weight);
  activity_info_insert.BindInt64(5, info->reconcile_stamp);
  activity_info_insert.BindInt(6, info->visits);

  return activity_info_insert.Run();
}

bool PublisherInfoDatabase::InsertOrUpdateActivityInfos(
    ledger::PublisherInfoList list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  CHECK(list);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized || filter.is_null()) {
    return false;
  }

  std::string query = "SELECT ai.publisher_id, ai.duration, ai.score, "
                      "ai.percent, ai.weight, spi.status, pi.excluded, "
                      "pi.name, pi.url, pi.provider, "
                      "pi.favIcon, ai.reconcile_stamp, ai.visits "
                      "FROM activity_info AS ai "
                      "INNER JOIN publisher_info AS pi "
                      "ON ai.publisher_id = pi.publisher_id "
                      "LEFT JOIN server_publisher_info AS spi "
                      "ON spi.publisher_key = pi.publisher_id "
                      "WHERE 1 = 1";

  if (!filter->id.empty()) {
    query += " AND ai.publisher_id = ?";
  }

  if (filter->reconcile_stamp > 0) {
    query += " AND ai.reconcile_stamp = ?";
  }

  if (filter->min_duration > 0) {
    query += " AND ai.duration >= ?";
  }

  if (filter->excluded != ledger::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
        ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded = ?";
  }

  if (filter->excluded ==
    ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded != ?";
  }

  if (filter->percent > 0) {
    query += " AND ai.percent >= ?";
  }

  if (filter->min_visits > 0) {
    query += " AND ai.visits >= ?";
  }

  if (!filter->non_verified) {
    const std::string status = base::StringPrintf(
        " AND spi.status != %1d",
        ledger::mojom::PublisherStatus::NOT_VERIFIED);
    query += status;
  }

  for (const auto& it : filter->order_by) {
    query += " ORDER BY " + it->property_name;
    query += (it->ascending ? " ASC" : " DESC");
  }

  if (limit > 0) {
    query += " LIMIT " + std::to_string(limit);

    if (start > 1) {
      query += " OFFSET " + std::to_string(start);
    }
  }

  sql::Statement info_sql(db_.GetUniqueStatement(query.c_str()));

  int column = 0;
  if (!filter->id.empty()) {
    info_sql.BindString(column++, filter->id);
  }

  if (filter->reconcile_stamp > 0) {
    info_sql.BindInt64(column++, filter->reconcile_stamp);
  }

  if (filter->min_duration > 0) {
    info_sql.BindInt(column++, filter->min_duration);
  }

  if (filter->excluded != ledger::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    info_sql.BindInt(column++, static_cast<int32_t>(filter->excluded));
  }

  if (filter->excluded ==
      ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    info_sql.BindInt(column++,
        static_cast<int>(ledger::PublisherExclude::EXCLUDED));
  }

  if (filter->percent > 0) {
    info_sql.BindInt(column++, filter->percent);
  }

  if (filter->min_visits > 0) {
    info_sql.BindInt(column++, filter->min_visits);
  }

  while (info_sql.Step()) {
    auto info = ledger::PublisherInfo::New();
    info->id = info_sql.ColumnString(0);
    info->duration = info_sql.ColumnInt64(1);
    info->score = info_sql.ColumnDouble(2);
    info->percent = info_sql.ColumnInt64(3);
    info->weight = info_sql.ColumnDouble(4);
    info->status =
        static_cast<ledger::mojom::PublisherStatus>(info_sql.ColumnInt64(5));
    info->excluded = static_cast<ledger::PublisherExclude>(
        info_sql.ColumnInt(6));
    info->name = info_sql.ColumnString(7);
    info->url = info_sql.ColumnString(8);
    info->provider = info_sql.ColumnString(9);
    info->favicon_url = info_sql.ColumnString(10);
    info->reconcile_stamp = info_sql.ColumnInt64(11);
    info->visits = info_sql.ColumnInt(12);

    list->push_back(std::move(info));
  }

  return true;
}

bool PublisherInfoDatabase::DeleteActivityInfo(
    const std::string& publisher_key,
    uint64_t reconcile_stamp) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized || publisher_key.empty() || reconcile_stamp == 0) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(
      SQL_FROM_HERE,
      "DELETE FROM activity_info WHERE "
      "publisher_id = ? AND reconcile_stamp = ?"));

  statement.BindString(0, publisher_key);
  statement.BindInt64(1, reconcile_stamp);

  return statement.Run();
}

/**
 *
 * MEDIA PUBLISHER INFO
 *
 */
bool PublisherInfoDatabase::InsertOrUpdateMediaPublisherInfo(
    const std::string& media_key,
    const std::string& publisher_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return media_publisher_info_->InsertOrUpdate(
      &GetDB(),
      media_key,
      publisher_key);
}

ledger::PublisherInfoPtr
PublisherInfoDatabase::GetMediaPublisherInfo(const std::string& media_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return recurring_tip_->InsertOrUpdate(&GetDB(), std::move(info));
}

void PublisherInfoDatabase::GetRecurringTips(
    ledger::PublisherInfoList* list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return;
  }

  recurring_tip_->GetAllRecords(&GetDB(), list);
}

bool PublisherInfoDatabase::RemoveRecurringTip(
    const std::string& publisher_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return pending_contribution_->InsertOrUpdate(&GetDB(), std::move(list));
}

double PublisherInfoDatabase::GetReservedAmount() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  double amount = 0.0;

  if (!initialized) {
    return amount;
  }

  return pending_contribution_->GetReservedAmount(&GetDB());
}

void PublisherInfoDatabase::GetPendingContributions(
    ledger::PendingContributionInfoList* list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return;
  }

  return pending_contribution_->GetAllRecords(&GetDB(), list);
}

bool PublisherInfoDatabase::RemovePendingContributions(
    const std::string& publisher_key,
    const std::string& viewing_id,
    uint64_t added_date) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return pending_contribution_->DeleteRecord(
      &GetDB(),
      publisher_key,
      viewing_id,
      added_date);
}

bool PublisherInfoDatabase::RemoveAllPendingContributions() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return server_publisher_info_->ClearAndInsertList(&GetDB(), list);
}

ledger::ServerPublisherInfoPtr PublisherInfoDatabase::GetServerPublisherInfo(
    const std::string& publisher_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return contribution_queue_->InsertOrUpdate(&GetDB(), std::move(info));
}

ledger::ContributionQueuePtr
PublisherInfoDatabase::GetFirstContributionQueue() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return nullptr;
  }

  return contribution_queue_->GetFirstRecord(&GetDB());
}

bool PublisherInfoDatabase::DeleteContributionQueue(const uint64_t id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return promotion_->InsertOrUpdate(&GetDB(), std::move(info));
}

ledger::PromotionPtr
PublisherInfoDatabase::GetPromotion(const std::string& id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return nullptr;
  }

  return promotion_->GetRecord(&GetDB(), id);
}

ledger::PromotionMap PublisherInfoDatabase::GetAllPromotions() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
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
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return unblinded_token_->InsertOrUpdate(&GetDB(), std::move(info));
}

ledger::UnblindedTokenList
PublisherInfoDatabase::GetAllUnblindedTokens()  {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    ledger::UnblindedTokenList empty_list;
    return empty_list;
  }

  return unblinded_token_->GetAllRecords(&GetDB());
}

bool PublisherInfoDatabase::DeleteUnblindedTokens(
    const std::vector<std::string>& id_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  return unblinded_token_->DeleteRecords(&GetDB(), id_list);
}

// Other -------------------------------------------------------------------

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

  std::string sql;

  // Activity info
  const char* activity = "activity_info";
  if (GetDB().DoesTableExist(activity)) {
    const char* column = "reconcile_stamp";
    if (!GetDB().DoesColumnExist(activity, column)) {
      sql.append(" ALTER TABLE ");
      sql.append(activity);
      sql.append(" ADD reconcile_stamp INTEGER DEFAULT 0 NOT NULL; ");
    }
  }

  // Contribution info
  const char* contribution = "contribution_info";
  if (GetDB().DoesTableExist(contribution)) {
    sql.append(" DROP TABLE ");
    sql.append(contribution);
    sql.append(" ; ");
  }

  if (!GetDB().Execute(sql.c_str())) {
    return false;
  }

  const char* name = "contribution_info";
  sql = "CREATE TABLE ";
  sql.append(name);
  sql.append(
      "("
      "publisher_id LONGVARCHAR,"
      "probi TEXT \"0\"  NOT NULL,"
      "date INTEGER NOT NULL,"
      "category INTEGER NOT NULL,"
      "month INTEGER NOT NULL,"
      "year INTEGER NOT NULL,"
      "CONSTRAINT fk_contribution_info_publisher_id"
      "    FOREIGN KEY (publisher_id)"
      "    REFERENCES publisher_info (publisher_id)"
      "    ON DELETE CASCADE)");
  if (!GetDB().Execute(sql.c_str())) {
    return false;
  }

  if (!GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS contribution_info_publisher_id_index "
      "ON contribution_info (publisher_id)")) {
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

  // Activity info
  const char* name = "activity_info";
  if (GetDB().DoesTableExist(name)) {
    std::string sql = "ALTER TABLE activity_info RENAME TO activity_info_old;";
    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }

    sql = "DROP INDEX activity_info_publisher_id_index;";
    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }

    sql = "CREATE TABLE ";
    sql.append(name);
    sql.append(
        "("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "visits INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "month INTEGER NOT NULL,"
        "year INTEGER NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT activity_unique "
        "UNIQUE (publisher_id, month, year, reconcile_stamp) "
        "CONSTRAINT fk_activity_info_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE)");
    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }

    if (!GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS activity_info_publisher_id_index "
      "ON activity_info (publisher_id)")) {
      return false;
    }

    std::string columns = "publisher_id, "
                          "duration, "
                          "score, "
                          "percent, "
                          "weight, "
                          "month, "
                          "year, "
                          "reconcile_stamp";

    sql = "PRAGMA foreign_keys=off;";
    sql.append("INSERT INTO activity_info (" + columns + ") "
               "SELECT " + columns + " "
               "FROM activity_info_old;");
    sql.append("UPDATE activity_info SET visits=5;");
    sql.append("DROP TABLE activity_info_old;");
    sql.append("PRAGMA foreign_keys=on;");

    return GetDB().Execute(sql.c_str());
  }

  return false;
}

bool PublisherInfoDatabase::MigrateV4toV5() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  sql::Statement info_sql(db_.GetUniqueStatement(
      "SELECT publisher_id, month, year, reconcile_stamp "
      "FROM activity_info "
      "WHERE visits = 0"));

  while (info_sql.Step()) {
    sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "UPDATE activity_info SET visits = 1 "
      "WHERE publisher_id = ? AND month = ? AND "
      "year = ? AND reconcile_stamp = ?"));

    statement.BindString(0, info_sql.ColumnString(0));
    statement.BindInt(1, info_sql.ColumnInt(1));
    statement.BindInt(2, info_sql.ColumnInt(2));
    statement.BindInt64(3, info_sql.ColumnInt64(3));
    statement.Run();
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV5toV6() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  const char* name = "activity_info";
  if (GetDB().DoesTableExist(name)) {
    std::string sql = "ALTER TABLE activity_info RENAME TO activity_info_old;";
    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }

    sql = "DROP INDEX activity_info_publisher_id_index;";
    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }

    sql = "CREATE TABLE ";
    sql.append(name);
    sql.append(
        "("
        "publisher_id LONGVARCHAR NOT NULL,"
        "duration INTEGER DEFAULT 0 NOT NULL,"
        "visits INTEGER DEFAULT 0 NOT NULL,"
        "score DOUBLE DEFAULT 0 NOT NULL,"
        "percent INTEGER DEFAULT 0 NOT NULL,"
        "weight DOUBLE DEFAULT 0 NOT NULL,"
        "reconcile_stamp INTEGER DEFAULT 0 NOT NULL,"
        "CONSTRAINT activity_unique "
        "UNIQUE (publisher_id, reconcile_stamp) "
        "CONSTRAINT fk_activity_info_publisher_id"
        "    FOREIGN KEY (publisher_id)"
        "    REFERENCES publisher_info (publisher_id)"
        "    ON DELETE CASCADE)");

    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }

    if (!GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS activity_info_publisher_id_index "
      "ON activity_info (publisher_id)")) {
      return false;
    }

    const std::string columns_insert = "publisher_id, "
                                       "duration, "
                                       "visits, "
                                       "score, "
                                       "percent, "
                                       "weight, "
                                       "reconcile_stamp";

    const std::string columns_select = "publisher_id, "
                                       "sum(duration) as duration, "
                                       "sum(visits) as visits, "
                                       "sum(score) as score, "
                                       "sum(percent) as percent, "
                                       "sum(weight) as weight, "
                                       "reconcile_stamp";

    sql = "PRAGMA foreign_keys=off;";
    sql.append("INSERT INTO activity_info (" + columns_insert + ") "
               "SELECT " + columns_select + " "
               "FROM activity_info_old "
               "GROUP BY publisher_id, reconcile_stamp;");
    sql.append("DROP TABLE activity_info_old;");
    sql.append("PRAGMA foreign_keys=on;");

    bool result = GetDB().Execute(sql.c_str());

    if (!result) {
      LOG(ERROR) << "DB: Error with MigrateV5toV6";
    }
  }

  return transaction.Commit();
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
    default:
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

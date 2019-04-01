/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/publisher_info_database.h"

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
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/recurring_donation.h"

namespace brave_rewards {

namespace {

const int kCurrentVersionNumber = 6;
const int kCompatibleVersionNumber = 1;

}  // namespace

PublisherInfoDatabase::PublisherInfoDatabase(const base::FilePath& db_path) :
    db_path_(db_path),
    initialized_(false),
    testing_current_version_(-1) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
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

  if (!CreatePublisherInfoTable() ||
      !CreateContributionInfoTable() ||
      !CreateActivityInfoTable() ||
      !CreateMediaPublisherInfoTable() ||
      !CreateRecurringDonationTable() ||
      !CreatePendingContributionsTable()) {
    return false;
  }

  CreateContributionInfoIndex();
  CreateActivityInfoIndex();
  CreateRecurringDonationIndex();
  CreatePendingContributionsIndex();

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

bool PublisherInfoDatabase::CreateContributionInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "contribution_info";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

  std::string sql;
  sql.append("CREATE TABLE ");
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

  return GetDB().Execute(sql.c_str());
}

bool PublisherInfoDatabase::CreateContributionInfoIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS contribution_info_publisher_id_index "
      "ON contribution_info (publisher_id)");
}

bool PublisherInfoDatabase::InsertContributionInfo(
    const brave_rewards::ContributionInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "INSERT INTO contribution_info "
      "(publisher_id, probi, date, "
      "category, month, year) "
      "VALUES (?, ?, ?, ?, ?, ?)"));

  statement.BindString(0, info.publisher_key);
  statement.BindString(1, info.probi);
  statement.BindInt64(2, info.date);
  statement.BindInt(3, info.category);
  statement.BindInt(4, info.month);
  statement.BindInt(5, info.year);

  return statement.Run();
}

void PublisherInfoDatabase::GetOneTimeTips(ledger::PublisherInfoList* list,
                                           ledger::ACTIVITY_MONTH month,
                                           int year) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return;
  }

  sql::Statement info_sql(db_.GetUniqueStatement(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "ci.probi, ci.date, pi.verified, pi.provider "
      "FROM contribution_info as ci "
      "INNER JOIN publisher_info AS pi ON ci.publisher_id = pi.publisher_id "
      "AND ci.month = ? AND ci.year = ? "
      "AND (ci.category = ? OR ci.category = ?)"));

  info_sql.BindInt(0, month);
  info_sql.BindInt(1, year);
  info_sql.BindInt(2, ledger::REWARDS_CATEGORY::DIRECT_DONATION);
  info_sql.BindInt(3, ledger::REWARDS_CATEGORY::TIPPING);

  while (info_sql.Step()) {
    std::string id(info_sql.ColumnString(0));

    ledger::PublisherInfo publisher(id);

    publisher.name = info_sql.ColumnString(1);
    publisher.url = info_sql.ColumnString(2);
    publisher.favicon_url = info_sql.ColumnString(3);
    publisher.weight = info_sql.ColumnDouble(4);
    publisher.reconcile_stamp = info_sql.ColumnInt64(5);
    publisher.verified = info_sql.ColumnBool(6);
    publisher.provider = info_sql.ColumnString(7);

    list->push_back(publisher);
  }
}

/**
 *
 * PUBLISHER INFO
 *
 */

bool PublisherInfoDatabase::CreatePublisherInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "publisher_info";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "publisher_id LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
      "verified BOOLEAN DEFAULT 0 NOT NULL,"
      "excluded INTEGER DEFAULT 0 NOT NULL,"
      "name TEXT NOT NULL,"
      "favIcon TEXT NOT NULL,"
      "url TEXT NOT NULL,"
      "provider TEXT NOT NULL)");

  return GetDB().Execute(sql.c_str());
}

bool PublisherInfoDatabase::InsertOrUpdatePublisherInfo(
    const ledger::PublisherInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized || info.id.empty()) {
    return false;
  }

  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  sql::Statement publisher_info_statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE,
                                 "INSERT OR REPLACE INTO publisher_info "
                                 "(publisher_id, verified, excluded, "
                                 "name, url, provider, favIcon) "
                                 "VALUES (?, ?, ?, ?, ?, ?, "
                                 "(SELECT IFNULL( "
                                 "(SELECT favicon FROM publisher_info "
                                 "WHERE publisher_id = ?), \"\"))"
                                 ")"));

  publisher_info_statement.BindString(0, info.id);
  publisher_info_statement.BindBool(1, info.verified);
  publisher_info_statement.BindInt(2, static_cast<int>(info.excluded));
  publisher_info_statement.BindString(3, info.name);
  publisher_info_statement.BindString(4, info.url);
  publisher_info_statement.BindString(5, info.provider);
  publisher_info_statement.BindString(6, info.id);

  publisher_info_statement.Run();

  std::string favicon = info.favicon_url;
  if (!favicon.empty()) {
    sql::Statement favicon_statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE,
                                 "UPDATE publisher_info SET favIcon = ? "
                                 "WHERE publisher_id = ?"));

    if (favicon == ledger::_clear_favicon) {
      favicon.clear();
    }

    favicon_statement.BindString(0, favicon);
    favicon_statement.BindString(1, info.id);

    favicon_statement.Run();
  }

  return transaction.Commit();
}

std::unique_ptr<ledger::PublisherInfo>
PublisherInfoDatabase::GetPublisherInfo(const std::string& publisher_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return nullptr;
  }

  sql::Statement info_sql(db_.GetUniqueStatement(
      "SELECT publisher_id, name, url, favIcon, provider, verified, excluded "
      "FROM publisher_info WHERE publisher_id=?"));

  info_sql.BindString(0, publisher_key);

  if (info_sql.Step()) {
    std::unique_ptr<ledger::PublisherInfo> info;
    info.reset(new ledger::PublisherInfo());
    info->id = info_sql.ColumnString(0);
    info->name = info_sql.ColumnString(1);
    info->url = info_sql.ColumnString(2);
    info->favicon_url = info_sql.ColumnString(3);
    info->provider = info_sql.ColumnString(4);
    info->verified = info_sql.ColumnBool(5);
    info->excluded = static_cast<ledger::PUBLISHER_EXCLUDE>(
        info_sql.ColumnInt(6));

    return info;
  }

  return nullptr;
}

std::unique_ptr<ledger::PublisherInfo>
PublisherInfoDatabase::GetPanelPublisher(
    const ledger::ActivityInfoFilter& filter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized || filter.id.empty()) {
    return nullptr;
  }

  sql::Statement info_sql(db_.GetUniqueStatement(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "pi.provider, pi.verified, pi.excluded, "
      "("
      "SELECT IFNULL(percent, 0) FROM activity_info WHERE "
      "publisher_id = ? AND reconcile_stamp = ? "
      ") as percent "
      "FROM publisher_info AS pi WHERE pi.publisher_id = ? LIMIT 1"));

  info_sql.BindString(0, filter.id);
  info_sql.BindInt64(1, filter.reconcile_stamp);
  info_sql.BindString(2, filter.id);

  if (info_sql.Step()) {
    std::unique_ptr<ledger::PublisherInfo> info;
    info.reset(new ledger::PublisherInfo());
    info->id = info_sql.ColumnString(0);
    info->name = info_sql.ColumnString(1);
    info->url = info_sql.ColumnString(2);
    info->favicon_url = info_sql.ColumnString(3);
    info->provider = info_sql.ColumnString(4);
    info->verified = info_sql.ColumnBool(5);
    info->excluded = static_cast<ledger::PUBLISHER_EXCLUDE>(
        info_sql.ColumnInt(6));
    info->percent = info_sql.ColumnInt(7);

    return info;
  }

  return nullptr;
}

bool PublisherInfoDatabase::RestorePublishers() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  sql::Statement restore_q(db_.GetUniqueStatement(
      "UPDATE publisher_info SET excluded=? WHERE excluded=?"));

  restore_q.BindInt(0, static_cast<int>(
      ledger::PUBLISHER_EXCLUDE::DEFAULT));
  restore_q.BindInt(1, static_cast<int>(
      ledger::PUBLISHER_EXCLUDE::EXCLUDED));

  return restore_q.Run();
}

int PublisherInfoDatabase::GetExcludedPublishersCount() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return 0;
  }

  sql::Statement query(db_.GetUniqueStatement(
      "SELECT COUNT(*) FROM publisher_info WHERE excluded=?"));

  query.BindInt(0, static_cast<int>(
      ledger::PUBLISHER_EXCLUDE::EXCLUDED));

  if (query.Step()) {
    return query.ColumnInt(0);
  }

  return 0;
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
    const ledger::PublisherInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized || info.id.empty()) {
    return false;
  }

  if (!InsertOrUpdatePublisherInfo(info)) {
    return false;
  }

  sql::Statement activity_info_insert(
    GetDB().GetCachedStatement(SQL_FROM_HERE,
        "INSERT OR REPLACE INTO activity_info "
        "(publisher_id, duration, score, percent, "
        "weight, reconcile_stamp, visits) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));

  activity_info_insert.BindString(0, info.id);
  activity_info_insert.BindInt64(1, static_cast<int>(info.duration));
  activity_info_insert.BindDouble(2, info.score);
  activity_info_insert.BindInt64(3, static_cast<int>(info.percent));
  activity_info_insert.BindDouble(4, info.weight);
  activity_info_insert.BindInt64(5, info.reconcile_stamp);
  activity_info_insert.BindInt(6, info.visits);

  return activity_info_insert.Run();
}

bool PublisherInfoDatabase::InsertOrUpdateActivityInfos(
    const ledger::PublisherInfoList& list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized || list.size() == 0) {
    return false;
  }

  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  for (const auto& info : list) {
    if (!InsertOrUpdateActivityInfo(info)) {
      transaction.Rollback();
      return false;
    }
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::GetActivityList(
    int start,
    int limit,
    const ledger::ActivityInfoFilter& filter,
    ledger::PublisherInfoList* list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  CHECK(list);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  std::string query = "SELECT ai.publisher_id, ai.duration, ai.score, "
                      "ai.percent, ai.weight, pi.verified, pi.excluded, "
                      "pi.name, pi.url, pi.provider, "
                      "pi.favIcon, ai.reconcile_stamp, ai.visits "
                      "FROM activity_info AS ai "
                      "INNER JOIN publisher_info AS pi "
                      "ON ai.publisher_id = pi.publisher_id "
                      "WHERE 1 = 1";

  if (!filter.id.empty()) {
    query += " AND ai.publisher_id = ?";
  }

  if (filter.reconcile_stamp > 0) {
    query += " AND ai.reconcile_stamp = ?";
  }

  if (filter.min_duration > 0) {
    query += " AND ai.duration >= ?";
  }

  if (filter.excluded != ledger::EXCLUDE_FILTER::FILTER_ALL &&
      filter.excluded !=
        ledger::EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded = ?";
  }

  if (filter.excluded ==
    ledger::EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded != ?";
  }

  if (filter.percent > 0) {
    query += " AND ai.percent >= ?";
  }

  if (filter.min_visits > 0) {
    query += " AND ai.visits >= ?";
  }

  if (!filter.non_verified) {
    query += " AND pi.verified = 1";
  }

  for (const auto& it : filter.order_by) {
    query += " ORDER BY " + it.first;
    query += (it.second ? " ASC" : " DESC");
  }

  if (limit > 0) {
    query += " LIMIT " + std::to_string(limit);

    if (start > 1) {
      query += " OFFSET " + std::to_string(start);
    }
  }

  sql::Statement info_sql(db_.GetUniqueStatement(query.c_str()));

  int column = 0;
  if (!filter.id.empty()) {
    info_sql.BindString(column++, filter.id);
  }

  if (filter.reconcile_stamp > 0) {
    info_sql.BindInt64(column++, filter.reconcile_stamp);
  }

  if (filter.min_duration > 0) {
    info_sql.BindInt(column++, filter.min_duration);
  }

  if (filter.excluded != ledger::EXCLUDE_FILTER::FILTER_ALL &&
      filter.excluded !=
      ledger::EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED) {
    info_sql.BindInt(column++, filter.excluded);
  }

  if (filter.excluded ==
      ledger::EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED) {
    info_sql.BindInt(column++, ledger::PUBLISHER_EXCLUDE::EXCLUDED);
  }

  if (filter.percent > 0) {
    info_sql.BindInt(column++, filter.percent);
  }

  if (filter.min_visits > 0) {
    info_sql.BindInt(column++, filter.min_visits);
  }

  while (info_sql.Step()) {
    std::string id(info_sql.ColumnString(0));

    ledger::PublisherInfo info(id);
    info.duration = info_sql.ColumnInt64(1);
    info.score = info_sql.ColumnDouble(2);
    info.percent = info_sql.ColumnInt64(3);
    info.weight = info_sql.ColumnDouble(4);
    info.verified = info_sql.ColumnBool(5);
    info.excluded = static_cast<ledger::PUBLISHER_EXCLUDE>(
        info_sql.ColumnInt(6));
    info.name = info_sql.ColumnString(7);
    info.url = info_sql.ColumnString(8);
    info.provider = info_sql.ColumnString(9);
    info.favicon_url = info_sql.ColumnString(10);
    info.reconcile_stamp = info_sql.ColumnInt64(11);
    info.visits = info_sql.ColumnInt(12);

    list->push_back(info);
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
bool PublisherInfoDatabase::CreateMediaPublisherInfoTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "media_publisher_info";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "media_key TEXT NOT NULL PRIMARY KEY UNIQUE,"
      "publisher_id LONGVARCHAR NOT NULL,"
      "CONSTRAINT fk_media_publisher_info_publisher_id"
      "    FOREIGN KEY (publisher_id)"
      "    REFERENCES publisher_info (publisher_id)"
      "    ON DELETE CASCADE)");

  return GetDB().Execute(sql.c_str());
}

bool PublisherInfoDatabase::InsertOrUpdateMediaPublisherInfo(
    const std::string& media_key, const std::string& publisher_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized || media_key.empty() || publisher_id.empty()) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(
      SQL_FROM_HERE,
      "INSERT OR REPLACE INTO media_publisher_info "
      "(media_key, publisher_id) "
      "VALUES (?, ?)"));

  statement.BindString(0, media_key);
  statement.BindString(1, publisher_id);

  return statement.Run();
}

std::unique_ptr<ledger::PublisherInfo>
PublisherInfoDatabase::GetMediaPublisherInfo(const std::string& media_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return nullptr;
  }

  sql::Statement info_sql(db_.GetUniqueStatement(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "pi.provider, pi.verified, pi.excluded "
      "FROM media_publisher_info as mpi "
      "INNER JOIN publisher_info AS pi ON mpi.publisher_id = pi.publisher_id "
      "WHERE mpi.media_key=?"));

  info_sql.BindString(0, media_key);

  if (info_sql.Step()) {
    std::unique_ptr<ledger::PublisherInfo> info(new ledger::PublisherInfo());
    info->id = info_sql.ColumnString(0);
    info->name = info_sql.ColumnString(1);
    info->url = info_sql.ColumnString(2);
    info->favicon_url = info_sql.ColumnString(3);
    info->provider = info_sql.ColumnString(4);
    info->verified = info_sql.ColumnBool(5);
    info->excluded = static_cast<ledger::PUBLISHER_EXCLUDE>(
        info_sql.ColumnInt(6));

    return info;
  }

  return nullptr;
}

/**
 *
 * RECURRING DONATION
 *
 */
bool PublisherInfoDatabase::CreateRecurringDonationTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "recurring_donation";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE,"
      "amount DOUBLE DEFAULT 0 NOT NULL,"
      "added_date INTEGER DEFAULT 0 NOT NULL,"
      "CONSTRAINT fk_recurring_donation_publisher_id"
      "    FOREIGN KEY (publisher_id)"
      "    REFERENCES publisher_info (publisher_id)"
      "    ON DELETE CASCADE)");

  return GetDB().Execute(sql.c_str());
}

bool PublisherInfoDatabase::CreateRecurringDonationIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS recurring_donation_publisher_id_index "
      "ON recurring_donation (publisher_id)");
}

bool PublisherInfoDatabase::InsertOrUpdateRecurringDonation(
    const brave_rewards::RecurringDonation& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized || info.publisher_key.empty()) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(
      SQL_FROM_HERE,
      "INSERT OR REPLACE INTO recurring_donation "
      "(publisher_id, amount, added_date) "
      "VALUES (?, ?, ?)"));

  statement.BindString(0, info.publisher_key);
  statement.BindDouble(1, info.amount);
  statement.BindInt64(2, info.added_date);

  return statement.Run();
}

void PublisherInfoDatabase::GetRecurringDonations(
    ledger::PublisherInfoList* list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return;
  }

  sql::Statement info_sql(db_.GetUniqueStatement(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "rd.amount, rd.added_date, pi.verified, pi.provider "
      "FROM recurring_donation as rd "
      "INNER JOIN publisher_info AS pi ON rd.publisher_id = pi.publisher_id "));

  while (info_sql.Step()) {
    std::string id(info_sql.ColumnString(0));

    ledger::PublisherInfo publisher(id);
    publisher.name = info_sql.ColumnString(1);
    publisher.url = info_sql.ColumnString(2);
    publisher.favicon_url = info_sql.ColumnString(3);
    publisher.weight = info_sql.ColumnDouble(4);
    publisher.reconcile_stamp = info_sql.ColumnInt64(5);
    publisher.verified = info_sql.ColumnBool(6);
    publisher.provider = info_sql.ColumnString(7);

    list->push_back(publisher);
  }
}

bool PublisherInfoDatabase::RemoveRecurring(const std::string& publisher_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  sql::Statement statement(GetDB().GetCachedStatement(
      SQL_FROM_HERE,
      "DELETE FROM recurring_donation WHERE publisher_id = ?"));

  statement.BindString(0, publisher_key);

  return statement.Run();
}

/**
 *
 * PENDING CONTRIBUTION
 *
 */
bool PublisherInfoDatabase::CreatePendingContributionsTable() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* name = "pending_contribution";
  if (GetDB().DoesTableExist(name)) {
    return true;
  }

  std::string sql;
  sql.append("CREATE TABLE ");
  sql.append(name);
  sql.append(
      "("
      "publisher_id LONGVARCHAR NOT NULL,"
      "amount DOUBLE DEFAULT 0 NOT NULL,"
      "added_date INTEGER DEFAULT 0 NOT NULL,"
      "viewing_id LONGVARCHAR NOT NULL,"
      "category INTEGER NOT NULL,"
      "CONSTRAINT fk_pending_contribution_publisher_id"
      "    FOREIGN KEY (publisher_id)"
      "    REFERENCES publisher_info (publisher_id)"
      "    ON DELETE CASCADE)");
  return GetDB().Execute(sql.c_str());
}

bool PublisherInfoDatabase::CreatePendingContributionsIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return GetDB().Execute(
      "CREATE INDEX IF NOT EXISTS pending_contribution_publisher_id_index "
      "ON pending_contribution (publisher_id)");
}

bool PublisherInfoDatabase::InsertPendingContribution
(const ledger::PendingContributionList& list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  if (!initialized) {
    return false;
  }

  base::Time now = base::Time::Now();
  double now_seconds = now.ToDoubleT();

  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  for (const auto& item : list.list_) {
    sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE,
      "INSERT INTO pending_contribution "
      "(publisher_id, amount, added_date, viewing_id, category) "
      "VALUES (?, ?, ?, ?, ?)"));

    statement.BindString(0, item.publisher_key);
    statement.BindDouble(1, item.amount);
    statement.BindInt64(2, now_seconds);
    statement.BindString(3, item.viewing_id);
    statement.BindInt(4, item.category);
    statement.Run();
  }

  return transaction.Commit();
}

double PublisherInfoDatabase::GetReservedAmount() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool initialized = Init();
  DCHECK(initialized);

  double amount = 0.0;

  if (!initialized) {
    return amount;
  }

  sql::Statement info_sql(
      db_.GetUniqueStatement("SELECT sum(amount) FROM pending_contribution"));

  if (info_sql.Step()) {
    amount = info_sql.ColumnDouble(0);
  }

  return amount;
}

int PublisherInfoDatabase::GetCurrentVersion() {
  if (testing_current_version_ != -1) {
    return testing_current_version_;
  }

  return kCurrentVersionNumber;
}

void PublisherInfoDatabase::SetTestingCurrentVersion(int value) {
  testing_current_version_ = value;
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

  if (!CreateContributionInfoTable()) {
    return false;
  }

  if (!CreateContributionInfoIndex()) {
    return false;
  }

  if (!CreateRecurringDonationTable()) {
    return false;
  }

  return CreateRecurringDonationIndex();
}

bool PublisherInfoDatabase::MigrateV2toV3() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!CreatePendingContributionsTable()) {
    return false;
  }

  return CreatePendingContributionsIndex();
}

bool PublisherInfoDatabase::MigrateV3toV4() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Activity info
  const char* activity = "activity_info";
  if (GetDB().DoesTableExist(activity)) {
    std::string sql = "ALTER TABLE activity_info RENAME TO activity_info_old;";

    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }

    if (!CreateActivityInfoTable()) {
      return false;
    }

    if (!CreateActivityInfoIndex()) {
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

  const char* activity = "activity_info";
  if (GetDB().DoesTableExist(activity)) {
    std::string sql = "ALTER TABLE activity_info RENAME TO activity_info_old;";

    if (!GetDB().Execute(sql.c_str())) {
      return false;
    }

    if (!CreateActivityInfoTable()) {
      return false;
    }

    if (!CreateActivityInfoIndex()) {
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
                                       "sum(visits) as duration, "
                                       "sum(score) as score, "
                                       "percent, "
                                       "weight, "
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

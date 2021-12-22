/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated_learning/data_stores/ad_notification_timing_data_store.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_federated_learning/data_stores/data_store.h"
#include "sql/meta_table.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {

void BindSampleLogToStatement(
    const brave::federated_learning::AdNotificationTimingTaskLog&
        notification_timing_log,
    sql::Statement* s) {
  s->BindInt64(0, notification_timing_log.time.ToInternalValue());
  s->BindString(1, notification_timing_log.locale);
  s->BindInt(2, notification_timing_log.number_of_tabs);
  s->BindBool(3, notification_timing_log.click);
  s->BindInt64(4, notification_timing_log.creation_date.ToInternalValue());
}

}  // namespace

namespace brave {

namespace federated_learning {

// AdNotificationTimingTaskLog ---------------------------------------

AdNotificationTimingTaskLog::AdNotificationTimingTaskLog(
    const int id,
    const base::Time& time,
    const std::string& locale,
    const int number_of_tabs,
    const bool click,
    const base::Time& creation_date)
    : id(id),
      time(time),
      locale(locale),
      number_of_tabs(number_of_tabs),
      click(click),
      creation_date(creation_date) {}

AdNotificationTimingTaskLog::AdNotificationTimingTaskLog(
    const AdNotificationTimingTaskLog& other) = default;

AdNotificationTimingTaskLog::AdNotificationTimingTaskLog()
    : id(0),
      time(base::Time::Now()),
      locale(std::string()),
      number_of_tabs(0),
      click(false),
      creation_date(base::Time::Now()) {}

AdNotificationTimingTaskLog::~AdNotificationTimingTaskLog() {}

// AdNotificationTimingDataStore
// -----------------------------------------------------

AdNotificationTimingDataStore::AdNotificationTimingDataStore(
    const base::FilePath& database_path)
    : DataStore(database_path) {}

bool AdNotificationTimingDataStore::Init(const std::string& task_id,
                                         const std::string& task_name,
                                         const int max_number_of_records,
                                         const int max_retention_days) {
  return DataStore::Init(task_id, task_name, max_number_of_records,
                         max_retention_days);
}

bool AdNotificationTimingDataStore::AddLog(
    const AdNotificationTimingTaskLog& log) {
  sql::Statement s(db_.GetUniqueStatement(
      base::StringPrintf(
          "INSERT INTO %s (time, locale, number_of_tabs, click, creation_date) "
          "VALUES (?,?,?,?,?)",
          task_name_.c_str())
          .c_str()));
  BindSampleLogToStatement(log, &s);
  return s.Run();
}

void AdNotificationTimingDataStore::LoadLogs(
    IdToAdNotificationTimingTaskLogMap* notification_timing_logs) {
  DCHECK(notification_timing_logs);
  sql::Statement s(db_.GetUniqueStatement(
      base::StringPrintf("SELECT id, time, locale, number_of_tabs, click, "
                         "creation_date FROM %s",
                         task_name_.c_str())
          .c_str()));

  notification_timing_logs->clear();
  while (s.Step()) {
    AdNotificationTimingTaskLog ntl(
        s.ColumnInt(0),                                    // id
        base::Time::FromInternalValue(s.ColumnInt64(1)),   // time
        s.ColumnString(2),                                 // locale
        s.ColumnInt(3),                                    // number_of_tabs
        s.ColumnBool(4),                                   // click
        base::Time::FromInternalValue(s.ColumnInt64(5)));  // creation_date

    notification_timing_logs->insert(std::make_pair(s.ColumnInt(0), ntl));
  }
}

AdNotificationTimingDataStore::~AdNotificationTimingDataStore() {}

bool AdNotificationTimingDataStore::EnsureTable() {
  if (db_.DoesTableExist(task_name_))
    return true;

  sql::Transaction transaction(&db_);
  return transaction.Begin() &&
         db_.Execute(
             base::StringPrintf(
                 "CREATE TABLE %s (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "time INTEGER, locale TEXT, number_of_tabs INTEGER, "
                 "click BOOLEAN, creation_date INTEGER)",
                 task_name_.c_str())
                 .c_str()) &&
         transaction.Commit();
}

void AdNotificationTimingDataStore::EnforceRetentionPolicy() {
  DataStore::EnforceRetentionPolicy();
}

}  // namespace federated_learning
}  // namespace brave

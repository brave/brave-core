/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/data_stores/test_data_store.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_federated/data_stores/data_store.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {

void BindSampleLogToStatement(const brave_federated::TestTaskLog& test_log,
                              sql::Statement* s) {
  s->BindBool(0, test_log.label);
  s->BindInt64(1, test_log.creation_date.ToInternalValue());
}

}  // namespace

namespace brave_federated {

// TestTaskLog ---------------------------------------

TestTaskLog::TestTaskLog(const int id,
                         const bool label,
                         const base::Time& creation_date)
    : id(id), label(label), creation_date(creation_date) {}

TestTaskLog::TestTaskLog(const TestTaskLog& other) = default;

TestTaskLog::TestTaskLog()
    : id(0), label(false), creation_date(base::Time::Now()) {}

TestTaskLog::~TestTaskLog() {}

// TestDataStore
// -----------------------------------------------------

TestDataStore::TestDataStore(const base::FilePath& database_path)
    : DataStore(database_path) {}

bool TestDataStore::Init(const int task_id,
                         const std::string& task_name,
                         const int max_number_of_records,
                         const int max_retention_days) {
  return DataStore::Init(task_id, task_name, max_number_of_records,
                         max_retention_days);
}

bool TestDataStore::AddLog(const TestTaskLog& log) {
  sql::Statement s(db_.GetUniqueStatement(
      base::StringPrintf("INSERT INTO %s (label, creation_date) "
                         "VALUES (?,?)",
                         task_name_.c_str())
          .c_str()));
  BindSampleLogToStatement(log, &s);
  return s.Run();
}

void TestDataStore::LoadLogs(IdToTestTaskLogMap* test_logs) {
  DCHECK(test_logs);
  sql::Statement s(db_.GetUniqueStatement(
      base::StringPrintf("SELECT id, label, creation_date FROM %s",
                         task_name_.c_str())
          .c_str()));

  test_logs->clear();
  while (s.Step()) {
    TestTaskLog ntl(
        s.ColumnInt(0),                                    // id
        s.ColumnBool(1),                                   // label
        base::Time::FromInternalValue(s.ColumnInt64(2)));  // creation_date

    test_logs->insert(std::make_pair(s.ColumnInt(0), ntl));
  }
}

TestDataStore::~TestDataStore() {}

bool TestDataStore::EnsureTable() {
  if (db_.DoesTableExist(task_name_))
    return true;

  sql::Transaction transaction(&db_);
  return transaction.Begin() &&
         db_.Execute(
             base::StringPrintf(
                 "CREATE TABLE %s (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "label BOOLEAN, creation_date INTEGER)",
                 task_name_.c_str())
                 .c_str()) &&
         transaction.Commit();
}

void TestDataStore::EnforceRetentionPolicy() {
  DataStore::EnforceRetentionPolicy();
}

}  // namespace brave_federated

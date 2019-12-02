/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace braveledger_database {

namespace {

const int kCurrentVersionNumber = 1;
const int kCompatibleVersionNumber = 1;

}  // namespace

Database::Database(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

Database::~Database() {
}

void Database::Initialize() {
  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database:: Initialize 1";
  const std::string db_path = ledger_->GetDatabasePath();
  const auto file_path = base::FilePath(db_path);

  DCHECK(!file_path.empty());
  if (file_path.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Database file path is empty!";
    return;
  }


  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database:: Initialize 2";

  if (!db_.Open(file_path)) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database can't be opened!";
    return;
  }


  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database:: Initialize 3";

  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    return;
  }


  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database:: Initialize 4";

  if (!meta_table_.Init(&db_, GetCurrentVersion(), kCompatibleVersionNumber)) {
    return;
  }


  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database:: Initialize 5";

  if (!InitializeTables()) {
    return;
  }


  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database:: Initialize 6";

  if (!committer.Commit()) {
    return;
  }


  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database:: Initialize 7";

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
    base::Bind(&Database::OnMemoryPressure,
    base::Unretained(this))));
}

void Database::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

bool InsertOnFileTaskRunner(Database* database) {
  const std::string query =
      "INSERT INTO test_db (field_1, field_2) VALUES (?, ?)";

  sql::Statement statement(
      database->GetDB().GetUniqueStatement(query.c_str()));

  statement.BindInt(0, 5);
  statement.BindInt(1, 6);

  return statement.Run();
}

bool Database::InitializeTables() {
  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database:: InitializeTables 1";
  const std::string query =
      "CREATE TABLE IF NOT EXISTS test_db ("
        "field_1 INTEGER DEFAULT 0 NOT NULL,"
        "field_2 INTEGER DEFAULT 0 NOT NULL"
      ")";

  bool result = db_.Execute(query.c_str());
  if (!result) {
    return false;
  }
  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database:: InitializeTables 2";

  base::PostTaskAndReplyWithResult(
      ledger_->GetTaskRunner().get(),
      FROM_HERE,
      base::BindOnce(&InsertOnFileTaskRunner, base::Unretained(this)),
      base::BindOnce(&Database::OnInserted, base::Unretained(this)));

  return true;
}

void Database::OnInserted(bool success) {
  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "INSERT STATUS:" << success;
}

int Database::GetCurrentVersion() {
  if (testing_current_version_ != -1) {
    return testing_current_version_;
  }

  return kCurrentVersionNumber;
}

sql::Database& Database::GetDB() {
  return db_;
}

}  // namespace braveledger_database

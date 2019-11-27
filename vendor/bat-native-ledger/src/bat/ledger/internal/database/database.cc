/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace braveledger_database {

namespace {

const int kCurrentVersionNumber = 10;
const int kCompatibleVersionNumber = 1;

}  // namespace

Database::Database(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

Database::~Database() {
}

void Database::Initialize() {
  const std::string db_path = ledger_->GetDatabasePath();
  const auto file_path = base::FilePath(db_path);

  DCHECK(!file_path.empty());
  if (file_path.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Database file path is empty!";
    return;
  }

  if (!db_.Open(file_path)) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Database can't be opened!";
    return;
  }

  sql::Transaction committer(&db_);
  if (!committer.Begin()) {
    return;
  }

  if (!meta_table_.Init(&db_, GetCurrentVersion(), kCompatibleVersionNumber)) {
    return;
  }

  if (!InitializeTables()) {
    return;
  }

  if (!committer.Commit()) {
    return;
  }

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
    base::Bind(&Database::OnMemoryPressure,
    base::Unretained(this))));
}

void Database::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  db_.TrimMemory();
}

bool Database::InitializeTables() {
  return true;
}

int Database::GetCurrentVersion() {
  if (testing_current_version_ != -1) {
    return testing_current_version_;
  }

  return kCurrentVersionNumber;
}



}  // namespace braveledger_database

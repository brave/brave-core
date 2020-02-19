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

const int kCurrentVersionNumber = 15;
const int kCompatibleVersionNumber = 1;

}  // namespace

PublisherInfoDatabase::PublisherInfoDatabase(
    const base::FilePath& db_path,
    const int testing_current_version) :
    db_path_(db_path),
    initialized_(false),
    testing_current_version_(testing_current_version) {
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
  sql::Transaction committer(&GetDB());
  if (!committer.Begin()) {
    return false;
  }

  int table_version = 0;
  if (GetMetaTable().DoesTableExist(&GetDB())) {
    if (!InitMetaTable(GetCurrentVersion())) {
      return false;
    }

    table_version = GetTableVersionNumber();
  }

  // Version check.
  sql::InitStatus version_status = EnsureCurrentVersion(table_version);
  if (version_status != sql::INIT_OK || !committer.Commit()) {
    return false;
  }

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&PublisherInfoDatabase::OnMemoryPressure,
      base::Unretained(this))));

  initialized_ = true;
  return initialized_;
}

// Other -------------------------------------------------------------------

bool PublisherInfoDatabase::IsInitialized() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool initialized = Init();
  DCHECK(initialized);
  return initialized;
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

bool PublisherInfoDatabase::InitMetaTable(const int version) {
  return GetMetaTable().Init(
      &GetDB(),
      version,
      kCompatibleVersionNumber);
}

sql::MetaTable& PublisherInfoDatabase::GetMetaTable() {
  return meta_table_;
}

int PublisherInfoDatabase::GetTableVersionNumber() {
  return GetMetaTable().GetVersionNumber();
}

std::string PublisherInfoDatabase::GetSchema() {
  return db_.GetSchema();
}

// Migration -------------------------------------------------------------------

bool PublisherInfoDatabase::MigrateV0toV1() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return true;
}
bool PublisherInfoDatabase::MigrateV1toV2() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return true;
}

bool PublisherInfoDatabase::MigrateV2toV3() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return true;
}

bool PublisherInfoDatabase::MigrateV3toV4() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return true;
}

bool PublisherInfoDatabase::MigrateV4toV5() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return true;
}

bool PublisherInfoDatabase::MigrateV5toV6() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return true;
}

bool PublisherInfoDatabase::MigrateV6toV7() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return true;
}

bool PublisherInfoDatabase::MigrateV7toV8() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV8toV9() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV9toV10() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV10toV11() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV11toV12() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV12toV13() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV13toV14() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::MigrateV14toV15() {
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return false;
  }

  return transaction.Commit();
}

bool PublisherInfoDatabase::Migrate(int version) {
  switch (version) {
    case 1: {
      return MigrateV0toV1();
    }
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
    case 14: {
      return MigrateV13toV14();
    }
    case 15: {
      return MigrateV14toV15();
    }
    default:
      NOTREACHED();
      return false;
  }
}

sql::InitStatus PublisherInfoDatabase::EnsureCurrentVersion(
    const int old_version) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const int current_version = GetCurrentVersion();
  const int start_version = old_version + 1;

  int migrated_version = old_version;
  for (auto i = start_version; i <= current_version; i++) {
    if (!Migrate(i)) {
      LOG(ERROR) << "DB: Error with MigrateV" << (i - 1) << "toV" << i;
      break;
    }

    if (i == 1) {
      if (!InitMetaTable(i)) {
        return sql::INIT_FAILURE;
      }
    }

    migrated_version = i;
  }


  GetMetaTable().SetVersionNumber(migrated_version);
  return sql::INIT_OK;
}

}  // namespace brave_rewards

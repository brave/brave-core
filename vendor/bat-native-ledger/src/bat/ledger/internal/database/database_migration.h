/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_MIGRATION_H_
#define BRAVELEDGER_DATABASE_DATABASE_MIGRATION_H_

#include <memory>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_database {

class DatabaseActivityInfo;
class DatabasePublisherInfo;
class DatabaseRecurringTip;
class DatabaseServerPublisherInfo;

class DatabaseMigration {
 public:
  explicit DatabaseMigration(bat_ledger::LedgerImpl* ledger);
  ~DatabaseMigration();

  void Start(
      const int table_version,
      ledger::ResultCallback callback);

 private:
  bool Migrate(ledger::DBTransaction* transaction, const int target);

  std::unique_ptr<DatabaseActivityInfo> activity_info_;
  std::unique_ptr<DatabasePublisherInfo> publisher_info_;
  std::unique_ptr<DatabaseRecurringTip> recurring_tip_;
  std::unique_ptr<DatabaseServerPublisherInfo> server_publisher_info_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_database
#endif  // BRAVELEDGER_DATABASE_DATABASE_MIGRATION_H_

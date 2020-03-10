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
class DatabaseContributionInfo;
class DatabaseContributionQueue;
class DatabaseCredsBatch;
class DatabaseMediaPublisherInfo;
class DatabasePendingContribution;
class DatabasePromotion;
class DatabasePublisherInfo;
class DatabaseRecurringTip;
class DatabaseServerPublisherInfo;
class DatabaseSKUOrder;
class DatabaseSKUTransaction;
class DatabaseUnblindedToken;

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
  std::unique_ptr<DatabaseContributionInfo> contribution_info_;
  std::unique_ptr<DatabaseContributionQueue> contribution_queue_;
  std::unique_ptr<DatabaseCredsBatch> creds_batch_;
  std::unique_ptr<DatabaseMediaPublisherInfo> media_publisher_info_;
  std::unique_ptr<DatabasePendingContribution> pending_contribution_;
  std::unique_ptr<DatabasePromotion> promotion_;
  std::unique_ptr<DatabasePublisherInfo> publisher_info_;
  std::unique_ptr<DatabaseRecurringTip> recurring_tip_;
  std::unique_ptr<DatabaseServerPublisherInfo> server_publisher_info_;
  std::unique_ptr<DatabaseSKUOrder> sku_order_;
  std::unique_ptr<DatabaseSKUTransaction> sku_transaction_;
  std::unique_ptr<DatabaseUnblindedToken> unblinded_token_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_database
#endif  // BRAVELEDGER_DATABASE_DATABASE_MIGRATION_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_
#define BRAVELEDGER_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_server_publisher_banner.h"
#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseServerPublisherInfo: public DatabaseTable {
 public:
  explicit DatabaseServerPublisherInfo(bat_ledger::LedgerImpl* ledger);
  ~DatabaseServerPublisherInfo() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      const ledger::ServerPublisherInfo& server_info,
      ledger::ResultCallback callback);

  void GetRecord(
      const std::string& publisher_key,
      ledger::GetServerPublisherInfoCallback callback);

  void DeleteExpiredRecords(
      const int64_t max_age_seconds,
      ledger::ResultCallback callback);

 private:
  bool CreateTableV7(ledger::DBTransaction* transaction);

  bool CreateIndexV7(ledger::DBTransaction* transaction);

  bool MigrateToV7(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);

  bool CreateTableV28(ledger::DBTransaction* transaction);

  bool MigrateToV28(ledger::DBTransaction* transaction);

  void OnGetRecordBanner(
      ledger::PublisherBannerPtr banner,
      const std::string& publisher_key,
      ledger::GetServerPublisherInfoCallback callback);

  void OnGetRecord(
      ledger::DBCommandResponsePtr response,
      const std::string& publisher_key,
      const ledger::PublisherBanner& banner,
      ledger::GetServerPublisherInfoCallback callback);

  void OnExpiredRecordsSelected(
      ledger::DBCommandResponsePtr response,
      ledger::ResultCallback callback);

  std::unique_ptr<DatabaseServerPublisherBanner> banner_;
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_

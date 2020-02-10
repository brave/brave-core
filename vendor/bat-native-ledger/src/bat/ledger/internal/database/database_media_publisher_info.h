/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_MEDIA_PUBLISHER_INFO_H_
#define BRAVELEDGER_DATABASE_DATABASE_MEDIA_PUBLISHER_INFO_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseMediaPublisherInfo: public DatabaseTable {
 public:
  explicit DatabaseMediaPublisherInfo(bat_ledger::LedgerImpl* ledger);
  ~DatabaseMediaPublisherInfo() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      const std::string& media_key,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  void GetRecord(
      const std::string& media_key,
      ledger::PublisherInfoCallback callback);

 private:
  bool CreateTableV1(ledger::DBTransaction* transaction);

  bool CreateTableV15(ledger::DBTransaction* transaction);

  bool CreateIndexV15(ledger::DBTransaction* transaction);

  bool MigrateToV1(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);

  void OnGetRecord(
      ledger::DBCommandResponsePtr response,
      ledger::PublisherInfoCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_MEDIA_PUBLISHER_INFO_H_

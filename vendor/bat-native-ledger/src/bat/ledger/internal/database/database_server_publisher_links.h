/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_SERVER_PUBLISHER_LINKS_H_
#define BRAVELEDGER_DATABASE_DATABASE_SERVER_PUBLISHER_LINKS_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseServerPublisherLinks: public DatabaseTable {
 public:
  explicit DatabaseServerPublisherLinks(bat_ledger::LedgerImpl* ledger);
  ~DatabaseServerPublisherLinks() override;

  void InsertOrUpdate(
      ledger::DBTransaction* transaction,
      const ledger::ServerPublisherInfo& server_info);

  void DeleteRecords(
      ledger::DBTransaction* transaction,
      const std::string& publisher_key_list);

  void GetRecord(
      const std::string& publisher_key,
      ServerPublisherLinksCallback callback);

 private:
  void OnGetRecord(
      ledger::DBCommandResponsePtr response,
      ServerPublisherLinksCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_SERVER_PUBLISHER_LINKS_H_

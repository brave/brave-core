/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_SERVER_PUBLISHER_AMOUNTS_H_
#define BRAVELEDGER_DATABASE_DATABASE_SERVER_PUBLISHER_AMOUNTS_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseServerPublisherAmounts: public DatabaseTable {
 public:
  explicit DatabaseServerPublisherAmounts(LedgerImpl* ledger);
  ~DatabaseServerPublisherAmounts() override;

  void InsertOrUpdate(
      type::DBTransaction* transaction,
      const type::ServerPublisherInfo& server_info);

  void DeleteRecords(
      type::DBTransaction* transaction,
      const std::string& publisher_key_list);

  void GetRecord(
      const std::string& publisher_key,
      ServerPublisherAmountsCallback callback);

 private:
  void OnGetRecord(
      type::DBCommandResponsePtr response,
      ServerPublisherAmountsCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_SERVER_PUBLISHER_AMOUNTS_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_PROCESSED_PUBLISHER_H_
#define BRAVELEDGER_DATABASE_DATABASE_PROCESSED_PUBLISHER_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseProcessedPublisher : public DatabaseTable {
 public:
  explicit DatabaseProcessedPublisher(bat_ledger::LedgerImpl* ledger);
  ~DatabaseProcessedPublisher() override;

  void InsertOrUpdateList(
      const std::vector<std::string>& list,
      ledger::ResultCallback callback);

  void WasProcessed(
      const std::string& publisher_key,
      ledger::ResultCallback callback);

 private:
  void OnWasProcessed(
      ledger::DBCommandResponsePtr response,
      ledger::ResultCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_PROCESSED_PUBLISHER_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_EVENT_LOG_H_
#define BRAVELEDGER_DATABASE_DATABASE_EVENT_LOG_H_

#include <map>
#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseEventLog: public DatabaseTable {
 public:
  explicit DatabaseEventLog(LedgerImpl* ledger);
  ~DatabaseEventLog() override;

  void Insert(const std::string& key, const std::string& value);

  void InsertRecords(
      const std::map<std::string, std::string>& records,
      ledger::ResultCallback callback);

  // returns last 2000 records
  void GetLastRecords(ledger::GetEventLogsCallback callback);

 private:
  void OnGetAllRecords(
      type::DBCommandResponsePtr response,
      ledger::GetEventLogsCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_EVENT_LOG_H_

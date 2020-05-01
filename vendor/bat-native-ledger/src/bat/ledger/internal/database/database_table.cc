/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_table.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_database {

DatabaseTable::DatabaseTable(bat_ledger::LedgerImpl* ledger): ledger_(ledger) {
  DCHECK(ledger_);
}

DatabaseTable::~DatabaseTable() = default;

bool DatabaseTable::InsertIndex(
    ledger::DBTransaction* transaction,
    const std::string& table_name,
    const std::string& key) {
  DCHECK(!table_name.empty());
  DCHECK(!key.empty());

  if (!transaction) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "CREATE INDEX %s_%s_index ON %s (%s)",
      table_name.c_str(),
      key.c_str(),
      table_name.c_str(),
      key.c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

}  // namespace braveledger_database

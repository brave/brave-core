/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/database/database_publisher_prefix_list.h"

#include <tuple>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/publisher/prefix_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"

using std::placeholders::_1;
using braveledger_publisher::PrefixIterator;

namespace {

const char kTableName[] = "publisher_prefix_list";

constexpr size_t kHashPrefixSize = 4;
constexpr size_t kMaxInsertRecords = 100'000;

std::tuple<PrefixIterator, std::string, size_t> GetPrefixInsertList(
    PrefixIterator begin,
    PrefixIterator end) {
  DCHECK(begin != end);
  size_t count = 0;
  std::string values;
  PrefixIterator iter = begin;
  for (iter = begin;
       iter != end && count < kMaxInsertRecords;
       ++count, ++iter) {
    auto prefix = *iter;
    DCHECK(prefix.size() >= kHashPrefixSize);
    std::string hex = base::HexEncode(prefix.data(), kHashPrefixSize);
    values.append(base::StringPrintf("(x'%s'),", hex.c_str()));
  }
  // Remove last comma
  if (!values.empty()) {
    values.pop_back();
  }
  return {iter, std::move(values), count};
}

}  // namespace

namespace braveledger_database {

DatabasePublisherPrefixList::DatabasePublisherPrefixList(
    bat_ledger::LedgerImpl* ledger)
    : DatabaseTable(ledger) {}

DatabasePublisherPrefixList::~DatabasePublisherPrefixList() = default;

bool DatabasePublisherPrefixList::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);
  switch (target) {
    case 28:
      return MigrateToV28(transaction);
    default:
      return true;
  }
}

bool DatabasePublisherPrefixList::MigrateToV28(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!braveledger_database::DropTable(transaction, kTableName)) {
    return false;
  }

  if (!CreateTableV28(transaction)) {
    return false;
  }

  ledger_->ClearState(ledger::kStateServerPublisherListStamp);
  return true;
}

bool DatabasePublisherPrefixList::CreateTableV28(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = base::StringPrintf(
      "CREATE TABLE %s "
      "(hash_prefix BLOB PRIMARY KEY NOT NULL)",
      kTableName);

  transaction->commands.push_back(std::move(command));
  return true;
}

void DatabasePublisherPrefixList::Search(
    const std::string& publisher_key,
    ledger::SearchPublisherPrefixListCallback callback) {
  std::string hex = braveledger_publisher::GetHashPrefixInHex(
      publisher_key,
      kHashPrefixSize);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      "SELECT EXISTS(SELECT hash_prefix FROM %s WHERE hash_prefix = x'%s')",
      kTableName,
      hex.c_str());

  command->record_bindings = {
    ledger::DBCommand::RecordBindingType::BOOL_TYPE
  };

  auto transaction = ledger::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->RunDBTransaction(std::move(transaction), [callback](
      ledger::DBCommandResponsePtr response) {
    if (!response || !response->result ||
        response->status != ledger::DBCommandResponse::Status::RESPONSE_OK ||
        response->result->get_records().empty()) {
      BLOG(0, "Unexpected database result while searching "
          "publisher prefix list.");
      callback(false);
      return;
    }
    callback(GetBoolColumn(response->result->get_records()[0].get(), 0));
  });
}

void DatabasePublisherPrefixList::Reset(
    std::unique_ptr<braveledger_publisher::PrefixListReader> reader,
    ledger::ResultCallback callback) {
  if (reader_) {
    BLOG(1, "Publisher prefix list batch insert in progress");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }
  if (reader->empty()) {
    BLOG(0, "Cannot reset with an empty publisher prefix list");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }
  reader_ = std::move(reader);
  InsertNext(reader_->begin(), callback);
}

void DatabasePublisherPrefixList::InsertNext(
    PrefixIterator begin,
    ledger::ResultCallback callback) {
  DCHECK(reader_ && begin != reader_->end());

  auto transaction = ledger::DBTransaction::New();

  if (begin == reader_->begin()) {
    BLOG(1, "Clearing publisher prefixes table");
    auto command = ledger::DBCommand::New();
    command->type = ledger::DBCommand::Type::RUN;
    command->command = base::StringPrintf("DELETE FROM %s", kTableName);
    transaction->commands.push_back(std::move(command));
  }

  auto insert_tuple = GetPrefixInsertList(begin, reader_->end());

  BLOG(1, "Inserting " << std::get<size_t>(insert_tuple)
      << " records into publisher prefix table");

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s (hash_prefix) VALUES %s",
      kTableName,
      std::get<std::string>(insert_tuple).data());

  transaction->commands.push_back(std::move(command));

  auto iter = std::get<PrefixIterator>(insert_tuple);

  ledger_->RunDBTransaction(std::move(transaction), [this, iter, callback](
      ledger::DBCommandResponsePtr response) {
    if (!response ||
        response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
      reader_ = nullptr;
      callback(ledger::Result::LEDGER_ERROR);
      return;
    }

    if (iter == reader_->end()) {
      reader_ = nullptr;
      callback(ledger::Result::LEDGER_OK);
      return;
    }

    InsertNext(iter, callback);
  });
}

}  // namespace braveledger_database

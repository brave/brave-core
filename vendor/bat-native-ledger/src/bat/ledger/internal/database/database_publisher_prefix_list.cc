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

using std::placeholders::_1;

namespace {

const char kTableName[] = "publisher_prefix_list";

constexpr size_t kHashPrefixSize = 4;
constexpr size_t kMaxInsertRecords = 100'000;

std::tuple<ledger::publisher::PrefixIterator, std::string, size_t>
GetPrefixInsertList(
    ledger::publisher::PrefixIterator begin,
    ledger::publisher::PrefixIterator end) {
  DCHECK(begin != end);
  size_t count = 0;
  std::string values;
  ledger::publisher::PrefixIterator iter = begin;
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

namespace ledger {

namespace database {

DatabasePublisherPrefixList::DatabasePublisherPrefixList(
    LedgerImpl* ledger)
    : DatabaseTable(ledger) {}

DatabasePublisherPrefixList::~DatabasePublisherPrefixList() = default;

void DatabasePublisherPrefixList::Search(
    const std::string& publisher_key,
    SearchPublisherPrefixListCallback callback) {
  std::string hex = publisher::GetHashPrefixInHex(
      publisher_key,
      kHashPrefixSize);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      "SELECT EXISTS(SELECT hash_prefix FROM %s WHERE hash_prefix = x'%s')",
      kTableName,
      hex.c_str());

  command->record_bindings = {
    type::DBCommand::RecordBindingType::BOOL_TYPE
  };

  auto transaction = type::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      [callback](type::DBCommandResponsePtr response) {
        if (!response || !response->result ||
            response->status !=
              type::DBCommandResponse::Status::RESPONSE_OK ||
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
    std::unique_ptr<publisher::PrefixListReader> reader,
    ledger::ResultCallback callback) {
  if (reader_) {
    BLOG(1, "Publisher prefix list batch insert in progress");
    callback(type::Result::LEDGER_ERROR);
    return;
  }
  if (reader->empty()) {
    BLOG(0, "Cannot reset with an empty publisher prefix list");
    callback(type::Result::LEDGER_ERROR);
    return;
  }
  reader_ = std::move(reader);
  InsertNext(reader_->begin(), callback);
}

void DatabasePublisherPrefixList::InsertNext(
    publisher::PrefixIterator begin,
    ledger::ResultCallback callback) {
  DCHECK(reader_ && begin != reader_->end());

  auto transaction = type::DBTransaction::New();

  if (begin == reader_->begin()) {
    BLOG(1, "Clearing publisher prefixes table");
    auto command = type::DBCommand::New();
    command->type = type::DBCommand::Type::RUN;
    command->command = base::StringPrintf("DELETE FROM %s", kTableName);
    transaction->commands.push_back(std::move(command));
  }

  auto insert_tuple = GetPrefixInsertList(begin, reader_->end());

  BLOG(1, "Inserting " << std::get<size_t>(insert_tuple)
      << " records into publisher prefix table");

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s (hash_prefix) VALUES %s",
      kTableName,
      std::get<std::string>(insert_tuple).data());

  transaction->commands.push_back(std::move(command));

  auto iter = std::get<publisher::PrefixIterator>(insert_tuple);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      [this, iter, callback](type::DBCommandResponsePtr response) {
        if (!response ||
            response->status !=
              type::DBCommandResponse::Status::RESPONSE_OK) {
          reader_ = nullptr;
          callback(type::Result::LEDGER_ERROR);
          return;
        }

        if (iter == reader_->end()) {
          reader_ = nullptr;
          callback(type::Result::LEDGER_OK);
          return;
        }

        InsertNext(iter, callback);
      });
}

}  // namespace database
}  // namespace ledger

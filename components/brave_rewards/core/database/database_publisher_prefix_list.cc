/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/database/database_publisher_prefix_list.h"

#include <optional>
#include <tuple>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/publisher/prefix_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {

namespace {

const char kTableName[] = "publisher_prefix_list";

constexpr size_t kHashPrefixSize = 4;
constexpr size_t kMaxInsertRecords = 100'000;

std::tuple<publisher::PrefixIterator, std::string, size_t> GetPrefixInsertList(
    publisher::PrefixIterator begin,
    publisher::PrefixIterator end) {
  DCHECK(begin != end);
  size_t count = 0;
  std::string values;
  publisher::PrefixIterator iter = begin;
  for (iter = begin; iter != end && count < kMaxInsertRecords;
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

namespace database {

DatabasePublisherPrefixList::DatabasePublisherPrefixList(
    RewardsEngineImpl& engine)
    : DatabaseTable(engine) {}

DatabasePublisherPrefixList::~DatabasePublisherPrefixList() = default;

void DatabasePublisherPrefixList::Search(
    const std::string& publisher_key,
    SearchPublisherPrefixListCallback callback) {
  std::string hex =
      publisher::GetHashPrefixInHex(publisher_key, kHashPrefixSize);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      "SELECT EXISTS(SELECT hash_prefix FROM %s WHERE hash_prefix = x'%s')",
      kTableName, hex.c_str());

  command->record_bindings = {mojom::DBCommand::RecordBindingType::BOOL_TYPE};

  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePublisherPrefixList::OnSearch,
                     base::Unretained(this), std::move(callback)));
}

void DatabasePublisherPrefixList::OnSearch(
    SearchPublisherPrefixListCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response || !response->result ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK ||
      response->result->get_records().empty()) {
    BLOG(0,
         "Unexpected database result while searching "
         "publisher prefix list.");
    callback(false);
    return;
  }

  callback(GetBoolColumn(response->result->get_records()[0].get(), 0));
}

void DatabasePublisherPrefixList::Reset(publisher::PrefixListReader reader,
                                        LegacyResultCallback callback) {
  if (reader_) {
    BLOG(1, "Publisher prefix list batch insert in progress");
    callback(mojom::Result::FAILED);
    return;
  }
  if (reader.empty()) {
    BLOG(0, "Cannot reset with an empty publisher prefix list");
    callback(mojom::Result::FAILED);
    return;
  }
  reader_ = std::move(reader);
  InsertNext(reader_->begin(), callback);
}

void DatabasePublisherPrefixList::InsertNext(publisher::PrefixIterator begin,
                                             LegacyResultCallback callback) {
  DCHECK(reader_ && begin != reader_->end());

  auto transaction = mojom::DBTransaction::New();

  if (begin == reader_->begin()) {
    BLOG(1, "Clearing publisher prefixes table");
    auto command = mojom::DBCommand::New();
    command->type = mojom::DBCommand::Type::RUN;
    command->command = base::StringPrintf("DELETE FROM %s", kTableName);
    transaction->commands.push_back(std::move(command));
  }

  auto insert_tuple = GetPrefixInsertList(begin, reader_->end());

  BLOG(1, "Inserting " << std::get<size_t>(insert_tuple)
                       << " records into publisher prefix table");

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s (hash_prefix) VALUES %s", kTableName,
      std::get<std::string>(insert_tuple).data());

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePublisherPrefixList::OnInsertNext,
                     base::Unretained(this), std::move(callback),
                     std::get<publisher::PrefixIterator>(insert_tuple)));
}

void DatabasePublisherPrefixList::OnInsertNext(
    LegacyResultCallback callback,
    publisher::PrefixIterator iter,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    reader_ = std::nullopt;
    callback(mojom::Result::FAILED);
    return;
  }

  if (iter == reader_->end()) {
    reader_ = std::nullopt;
    callback(mojom::Result::OK);
    return;
  }

  InsertNext(iter, callback);
}

}  // namespace database
}  // namespace brave_rewards::internal

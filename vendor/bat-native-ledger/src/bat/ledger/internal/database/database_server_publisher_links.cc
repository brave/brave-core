/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_server_publisher_links.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "server_publisher_links";

}  // namespace

namespace ledger {
namespace database {

DatabaseServerPublisherLinks::DatabaseServerPublisherLinks(
    LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseServerPublisherLinks::~DatabaseServerPublisherLinks() = default;

void DatabaseServerPublisherLinks::InsertOrUpdate(
    type::DBTransaction* transaction,
    const type::ServerPublisherInfo& server_info) {
  DCHECK(transaction && !server_info.publisher_key.empty());

  if (!server_info.banner || server_info.banner->links.empty()) {
    return;
  }

  std::string value_list;
  for (auto& link : server_info.banner->links) {
    if (link.second.empty()) {
      continue;
    }
    value_list += base::StringPrintf(
        R"(('%s','%s','%s'),)",
        server_info.publisher_key.c_str(),
        link.first.c_str(),
        link.second.c_str());
  }

  if (value_list.empty()) {
    return;
  }

  // Remove trailing comma
  value_list.pop_back();

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s VALUES %s",
      kTableName,
      value_list.c_str());

  transaction->commands.push_back(std::move(command));
}

void DatabaseServerPublisherLinks::DeleteRecords(
    type::DBTransaction* transaction,
    const std::string& publisher_key_list) {
  DCHECK(transaction);
  if (publisher_key_list.empty()) {
    return;
  }

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "DELETE FROM %s WHERE publisher_key IN (%s)",
      kTableName,
      publisher_key_list.c_str());

  transaction->commands.push_back(std::move(command));
}

void DatabaseServerPublisherLinks::GetRecord(
    const std::string& publisher_key,
    ServerPublisherLinksCallback callback) {
  if (publisher_key.empty()) {
    BLOG(1, "Publisher key is empty");
    callback({});
    return;
  }

  auto transaction = type::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT provider, link FROM %s WHERE publisher_key=?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseServerPublisherLinks::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseServerPublisherLinks::OnGetRecord(
    type::DBCommandResponsePtr response,
    ServerPublisherLinksCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  std::map<std::string, std::string> links;
  for (auto const& record : response->result->get_records()) {
    auto* record_pointer = record.get();
    const auto pair = std::make_pair(
        GetStringColumn(record_pointer, 0),
        GetStringColumn(record_pointer, 1));
    links.insert(pair);
  }

  callback(links);
}

}  // namespace database
}  // namespace ledger

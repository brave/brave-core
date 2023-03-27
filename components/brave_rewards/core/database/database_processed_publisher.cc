/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_processed_publisher.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "processed_publisher";

}  // namespace

namespace brave_rewards::core {
namespace database {

DatabaseProcessedPublisher::DatabaseProcessedPublisher(LedgerImpl* ledger)
    : DatabaseTable(ledger) {}

DatabaseProcessedPublisher::~DatabaseProcessedPublisher() = default;

void DatabaseProcessedPublisher::InsertOrUpdateList(
    const std::vector<std::string>& list,
    LegacyResultCallback callback) {
  if (list.empty()) {
    BLOG(1, "List is empty");
    callback(mojom::Result::LEDGER_OK);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR IGNORE INTO %s (publisher_key) VALUES (?);", kTableName);

  for (const auto& publisher_key : list) {
    auto command = mojom::DBCommand::New();
    command->type = mojom::DBCommand::Type::RUN;
    command->command = query;

    BindString(command.get(), 0, publisher_key);

    transaction->commands.push_back(std::move(command));
  }

  auto transaction_callback = std::bind(&OnResultCallback, _1, callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseProcessedPublisher::WasProcessed(const std::string& publisher_key,
                                              LegacyResultCallback callback) {
  if (publisher_key.empty()) {
    BLOG(1, "Publisher key is empty");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT publisher_key FROM %s WHERE publisher_key = ?", kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(
      &DatabaseProcessedPublisher::OnWasProcessed, this, _1, callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseProcessedPublisher::OnWasProcessed(
    mojom::DBCommandResponsePtr response,
    LegacyResultCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  if (response->result->get_records().empty()) {
    callback(mojom::Result::NOT_FOUND);
    return;
  }

  callback(mojom::Result::LEDGER_OK);
}

}  // namespace database
}  // namespace brave_rewards::core

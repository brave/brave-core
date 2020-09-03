/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_contribution_queue_publishers.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace ledger {
namespace database {

namespace {

const char kTableName[] = "contribution_queue_publishers";

}  // namespace

DatabaseContributionQueuePublishers::DatabaseContributionQueuePublishers(
    LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseContributionQueuePublishers::
~DatabaseContributionQueuePublishers() = default;

void DatabaseContributionQueuePublishers::InsertOrUpdate(
    const std::string& id,
    type::ContributionQueuePublisherList list,
    ledger::ResultCallback callback) {
  if (id.empty() || list.empty()) {
    BLOG(1, "Empty data");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(contribution_queue_id, publisher_key, amount_percent) VALUES (?, ?, ?)",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  for (const auto& publisher : list) {
    BindString(command.get(), 0, id);
    BindString(command.get(), 1, publisher->publisher_key);
    BindDouble(command.get(), 2, publisher->amount_percent);

    transaction->commands.push_back(command->Clone());
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionQueuePublishers::GetRecordsByQueueId(
    const std::string& queue_id,
    ContributionQueuePublishersListCallback callback) {
  if (queue_id.empty()) {
    BLOG(1, "Queue id is empty");
    callback({});
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT publisher_key, amount_percent "
      "FROM %s WHERE contribution_queue_id = ?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, queue_id);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::DOUBLE_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionQueuePublishers::OnGetRecordsByQueueId,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionQueuePublishers::OnGetRecordsByQueueId(
    type::DBCommandResponsePtr response,
    ContributionQueuePublishersListCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  type::ContributionQueuePublisherList list;
  for (auto const& record : response->result->get_records()) {
    auto info = type::ContributionQueuePublisher::New();
    auto* record_pointer = record.get();

    info->publisher_key = GetStringColumn(record_pointer, 0);
    info->amount_percent = GetDoubleColumn(record_pointer, 1);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

}  // namespace database
}  // namespace ledger

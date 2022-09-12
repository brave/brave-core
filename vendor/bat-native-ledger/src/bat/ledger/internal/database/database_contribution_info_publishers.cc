/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_contribution_info_publishers.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace ledger {
namespace database {

namespace {

const char kTableName[] = "contribution_info_publishers";

}  // namespace

DatabaseContributionInfoPublishers::DatabaseContributionInfoPublishers(
    LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseContributionInfoPublishers::
~DatabaseContributionInfoPublishers() = default;

void DatabaseContributionInfoPublishers::InsertOrUpdate(
    mojom::DBTransaction* transaction,
    mojom::ContributionInfoPtr info) {
  DCHECK(transaction);

  if (!info) {
    BLOG(1, "Info is null");
    return;
  }

  const std::string query = base::StringPrintf(
    "INSERT OR REPLACE INTO %s "
    "(contribution_id, publisher_key, total_amount, contributed_amount) "
    "VALUES (?, ?, ?, ?)",
    kTableName);

  for (const auto& publisher : info->publishers) {
    auto command = mojom::DBCommand::New();
    command->type = mojom::DBCommand::Type::RUN;
    command->command = query;

    BindString(command.get(), 0, publisher->contribution_id);
    BindString(command.get(), 1, publisher->publisher_key);
    BindDouble(command.get(), 2, publisher->total_amount);
    BindDouble(command.get(), 3, publisher->contributed_amount);
    transaction->commands.push_back(std::move(command));
  }
}

void DatabaseContributionInfoPublishers::GetRecordByContributionList(
    const std::vector<std::string>& contribution_ids,
    ContributionPublisherListCallback callback) {
  if (contribution_ids.empty()) {
    BLOG(1, "Contribution ids is empty");
    callback({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT contribution_id, publisher_key, total_amount, contributed_amount "
    "FROM %s WHERE contribution_id IN (%s)",
    kTableName,
    GenerateStringInCase(contribution_ids).c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(
      &DatabaseContributionInfoPublishers::OnGetRecordByContributionList,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionInfoPublishers::OnGetRecordByContributionList(
    mojom::DBCommandResponsePtr response,
    ContributionPublisherListCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is not ok");
    callback({});
    return;
  }

  std::vector<mojom::ContributionPublisherPtr> list;
  for (auto const& record : response->result->get_records()) {
    auto info = mojom::ContributionPublisher::New();
    auto* record_pointer = record.get();

    info->contribution_id = GetStringColumn(record_pointer, 0);
    info->publisher_key = GetStringColumn(record_pointer, 1);
    info->total_amount = GetDoubleColumn(record_pointer, 2);
    info->contributed_amount = GetDoubleColumn(record_pointer, 3);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseContributionInfoPublishers::GetContributionPublisherPairList(
    const std::vector<std::string>& contribution_ids,
    ContributionPublisherPairListCallback callback) {
  if (contribution_ids.empty()) {
    BLOG(1, "Contribution ids is empty");
    callback({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT cip.contribution_id, cip.publisher_key, cip.total_amount, "
    "pi.name, pi.url, pi.favIcon, spi.status, spi.updated_at, pi.provider "
    "FROM %s as cip "
    "INNER JOIN publisher_info AS pi ON cip.publisher_key = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = cip.publisher_key "
    "WHERE cip.contribution_id IN (%s)",
    kTableName,
    GenerateStringInCase(contribution_ids).c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(
      &DatabaseContributionInfoPublishers::OnGetContributionPublisherInfoMap,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionInfoPublishers::OnGetContributionPublisherInfoMap(
    mojom::DBCommandResponsePtr response,
    ContributionPublisherPairListCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is not ok");
    callback({});
    return;
  }

  std::vector<ContributionPublisherInfoPair> pair_list;
  for (auto const& record : response->result->get_records()) {
    auto publisher = mojom::PublisherInfo::New();
    auto* record_pointer = record.get();

    publisher->id = GetStringColumn(record_pointer, 1);
    publisher->weight = GetDoubleColumn(record_pointer, 2);
    publisher->name = GetStringColumn(record_pointer, 3);
    publisher->url = GetStringColumn(record_pointer, 4);
    publisher->favicon_url = GetStringColumn(record_pointer, 5);
    publisher->status =
        static_cast<mojom::PublisherStatus>(GetInt64Column(record_pointer, 6));
    publisher->status_updated_at = GetInt64Column(record_pointer, 7);
    publisher->provider = GetStringColumn(record_pointer, 8);

    pair_list.emplace_back(GetStringColumn(record_pointer, 0),
                           std::move(publisher));
  }

  callback(std::move(pair_list));
}

void DatabaseContributionInfoPublishers::UpdateContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ledger::LegacyResultCallback callback) {
  if (contribution_id.empty() || publisher_key.empty()) {
    BLOG(1, "Data is empty " << contribution_id << "/" << publisher_key);
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET contributed_amount="
      "(SELECT total_amount WHERE contribution_id = ? AND publisher_key = ?) "
      "WHERE contribution_id = ? AND publisher_key = ?;",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, contribution_id);
  BindString(command.get(), 1, publisher_key);
  BindString(command.get(), 2, contribution_id);
  BindString(command.get(), 3, publisher_key);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace database
}  // namespace ledger

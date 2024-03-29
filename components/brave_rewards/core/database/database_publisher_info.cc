/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/database/database_publisher_info.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace {

constexpr char kTableName[] = "publisher_info";

}  // namespace

namespace brave_rewards::internal::database {

DatabasePublisherInfo::DatabasePublisherInfo(RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabasePublisherInfo::~DatabasePublisherInfo() = default;

void DatabasePublisherInfo::InsertOrUpdate(mojom::PublisherInfoPtr info,
                                           ResultCallback callback) {
  if (!info || info->id.empty()) {
    engine_->Log(FROM_HERE) << "Info is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, excluded, name, url, provider, favIcon) "
      "VALUES (?, ?, ?, ?, ?, "
      "(SELECT IFNULL( "
      "(SELECT favicon FROM %s "
      "WHERE publisher_id = ?), '')));",
      kTableName, kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, static_cast<int>(info->excluded));
  BindString(command.get(), 2, info->name);
  BindString(command.get(), 3, info->url);
  BindString(command.get(), 4, info->provider);
  BindString(command.get(), 5, info->id);

  transaction->commands.push_back(std::move(command));

  std::string favicon = info->favicon_url;
  if (!favicon.empty() && !info->provider.empty()) {
    const std::string query_icon = base::StringPrintf(
        "UPDATE %s SET favIcon = ? WHERE publisher_id = ?;", kTableName);

    auto command_icon = mojom::DBCommand::New();
    command_icon->type = mojom::DBCommand::Type::kRun;
    command_icon->command = query_icon;

    if (favicon == constant::kClearFavicon) {
      favicon.clear();
    }

    BindString(command_icon.get(), 0, favicon);
    BindString(command_icon.get(), 1, info->id);

    transaction->commands.push_back(std::move(command_icon));
  }

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabasePublisherInfo::GetRecord(const std::string& publisher_key,
                                      GetPublisherInfoCallback callback) {
  if (publisher_key.empty()) {
    engine_->Log(FROM_HERE) << "Publisher key is empty";
    std::move(callback).Run(mojom::Result::FAILED, {});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, pi.provider, "
      "spi.status, spi.updated_at, pi.excluded "
      "FROM %s as pi "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE publisher_id=?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt64,
                              mojom::DBCommand::RecordBindingType::kInt64,
                              mojom::DBCommand::RecordBindingType::kInt};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePublisherInfo::OnGetRecord,
                     base::Unretained(this), std::move(callback)));
}

void DatabasePublisherInfo::OnGetRecord(GetPublisherInfoCallback callback,
                                        mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run(mojom::Result::FAILED, {});
    return;
  }

  if (response->records.size() != 1) {
    std::move(callback).Run(mojom::Result::NOT_FOUND, {});
    return;
  }

  auto* record = response->records[0].get();

  auto info = mojom::PublisherInfo::New();
  info->id = GetStringColumn(record, 0);
  info->name = GetStringColumn(record, 1);
  info->url = GetStringColumn(record, 2);
  info->favicon_url = GetStringColumn(record, 3);
  info->provider = GetStringColumn(record, 4);
  info->status = PublisherStatusFromInt(GetInt64Column(record, 5));
  info->status_updated_at = GetInt64Column(record, 6);
  info->excluded = PublisherExcludeFromInt(GetIntColumn(record, 7));

  std::move(callback).Run(mojom::Result::OK, std::move(info));
}

void DatabasePublisherInfo::GetPanelRecord(
    mojom::ActivityInfoFilterPtr filter,
    GetPublisherPanelInfoCallback callback) {
  if (!filter || filter->id.empty()) {
    engine_->Log(FROM_HERE) << "Filter is empty";
    std::move(callback).Run(mojom::Result::FAILED, {});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "pi.provider, spi.status, pi.excluded, "
      "("
      "  SELECT IFNULL(percent, 0) FROM activity_info WHERE "
      "  publisher_id = ? AND reconcile_stamp = ? "
      ") as percent "
      "FROM %s AS pi "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE pi.publisher_id = ? LIMIT 1",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  BindString(command.get(), 0, filter->id);
  BindInt64(command.get(), 1, filter->reconcile_stamp);
  BindString(command.get(), 2, filter->id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt64,
                              mojom::DBCommand::RecordBindingType::kInt,
                              mojom::DBCommand::RecordBindingType::kInt};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePublisherInfo::OnGetPanelRecord,
                     base::Unretained(this), std::move(callback)));
}

void DatabasePublisherInfo::OnGetPanelRecord(
    GetPublisherPanelInfoCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run(mojom::Result::FAILED, {});
    return;
  }

  if (response->records.size() != 1) {
    std::move(callback).Run(mojom::Result::NOT_FOUND, {});
    return;
  }

  auto* record = response->records[0].get();

  auto info = mojom::PublisherInfo::New();
  info->id = GetStringColumn(record, 0);
  info->name = GetStringColumn(record, 1);
  info->url = GetStringColumn(record, 2);
  info->favicon_url = GetStringColumn(record, 3);
  info->provider = GetStringColumn(record, 4);
  info->status = PublisherStatusFromInt(GetInt64Column(record, 5));
  info->excluded = PublisherExcludeFromInt(GetIntColumn(record, 6));
  info->percent = GetIntColumn(record, 7);

  std::move(callback).Run(mojom::Result::OK, std::move(info));
}

void DatabasePublisherInfo::RestorePublishers(ResultCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "UPDATE %s SET excluded=? WHERE excluded=?", kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(mojom::PublisherExclude::DEFAULT));
  BindInt(command.get(), 1,
          static_cast<int>(mojom::PublisherExclude::EXCLUDED));

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePublisherInfo::OnRestorePublishers,
                     base::Unretained(this), std::move(callback)));
}

void DatabasePublisherInfo::OnRestorePublishers(
    ResultCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  engine_->publisher()->OnRestorePublishers(mojom::Result::OK,
                                            std::move(callback));
}

void DatabasePublisherInfo::GetExcludedList(GetExcludedListCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, spi.status, pi.name,"
      "pi.favicon, pi.url, pi.provider "
      "FROM %s as pi "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE pi.excluded = 1",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt64,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePublisherInfo::OnGetExcludedList,
                     base::Unretained(this), std::move(callback)));
}

void DatabasePublisherInfo::OnGetExcludedList(
    GetExcludedListCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::PublisherInfoPtr> list;
  for (auto const& record : response->records) {
    auto info = mojom::PublisherInfo::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->status = PublisherStatusFromInt(GetInt64Column(record_pointer, 1));
    info->name = GetStringColumn(record_pointer, 2);
    info->favicon_url = GetStringColumn(record_pointer, 3);
    info->url = GetStringColumn(record_pointer, 4);
    info->provider = GetStringColumn(record_pointer, 5);

    list.push_back(std::move(info));
  }

  std::move(callback).Run(std::move(list));
}

}  // namespace brave_rewards::internal::database

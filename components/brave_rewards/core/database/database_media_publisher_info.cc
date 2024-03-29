/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_media_publisher_info.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

constexpr char kTableName[] = "media_publisher_info";

}  // namespace

DatabaseMediaPublisherInfo::DatabaseMediaPublisherInfo(RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabaseMediaPublisherInfo::~DatabaseMediaPublisherInfo() = default;

void DatabaseMediaPublisherInfo::InsertOrUpdate(
    const std::string& media_key,
    const std::string& publisher_key,
    ResultCallback callback) {
  if (media_key.empty() || publisher_key.empty()) {
    engine_->Log(FROM_HERE)
        << "Data is empty " << media_key << "/" << publisher_key;
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s (media_key, publisher_id) VALUES (?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindString(command.get(), 0, media_key);
  BindString(command.get(), 1, publisher_key);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseMediaPublisherInfo::GetRecord(const std::string& media_key,
                                           PublisherInfoCallback callback) {
  if (media_key.empty()) {
    engine_->Log(FROM_HERE) << "Media key is empty";
    return std::move(callback).Run(mojom::Result::FAILED, {});
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "pi.provider, spi.status, spi.updated_at, pi.excluded "
      "FROM %s as mpi "
      "INNER JOIN publisher_info AS pi ON mpi.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE mpi.media_key=?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  BindString(command.get(), 0, media_key);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt,
                              mojom::DBCommand::RecordBindingType::kInt64,
                              mojom::DBCommand::RecordBindingType::kInt};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseMediaPublisherInfo::OnGetRecord,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseMediaPublisherInfo::OnGetRecord(
    PublisherInfoCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->Log(FROM_HERE) << "Response is wrong";
    std::move(callback).Run(mojom::Result::FAILED, {});
    return;
  }

  if (response->records.size() != 1) {
    engine_->Log(FROM_HERE)
        << "Record size is not correct: " << response->records.size();
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
  info->status = PublisherStatusFromInt(GetIntColumn(record, 5));
  info->status_updated_at = GetInt64Column(record, 6);
  info->excluded = PublisherExcludeFromInt(GetIntColumn(record, 7));

  std::move(callback).Run(mojom::Result::OK, std::move(info));
}

}  // namespace brave_rewards::internal::database

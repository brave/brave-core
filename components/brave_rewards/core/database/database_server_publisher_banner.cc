/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_server_publisher_banner.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace {

constexpr char kTableName[] = "server_publisher_banner";

}  // namespace

namespace brave_rewards::internal::database {

DatabaseServerPublisherBanner::DatabaseServerPublisherBanner(
    RewardsEngine& engine)
    : DatabaseTable(engine), links_(engine) {}

DatabaseServerPublisherBanner::~DatabaseServerPublisherBanner() = default;

void DatabaseServerPublisherBanner::InsertOrUpdate(
    mojom::DBTransaction* transaction,
    const mojom::ServerPublisherInfo& server_info) {
  DCHECK(transaction);
  DCHECK(!server_info.publisher_key.empty());

  // Do not insert a record if there is no banner data
  // or if banner data is empty.
  mojom::PublisherBanner default_banner;
  if (!server_info.banner || server_info.banner->Equals(default_banner)) {
    engine_->Log(FROM_HERE) << "Empty publisher banner data, skipping insert";
    return;
  }

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_key, title, description, background, logo, web3_url) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  BindString(command.get(), 0, server_info.publisher_key);
  BindString(command.get(), 1, server_info.banner->title);
  BindString(command.get(), 2, server_info.banner->description);
  BindString(command.get(), 3, server_info.banner->background);
  BindString(command.get(), 4, server_info.banner->logo);
  BindString(command.get(), 5, server_info.banner->web3_url);

  transaction->commands.push_back(std::move(command));

  links_.InsertOrUpdate(transaction, server_info);
}

void DatabaseServerPublisherBanner::DeleteRecords(
    mojom::DBTransaction* transaction,
    const std::string& publisher_key_list) {
  DCHECK(transaction);
  if (publisher_key_list.empty()) {
    return;
  }

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command =
      base::StringPrintf("DELETE FROM %s WHERE publisher_key IN (%s)",
                         kTableName, publisher_key_list.c_str());

  transaction->commands.push_back(std::move(command));

  links_.DeleteRecords(transaction, publisher_key_list);
}

void DatabaseServerPublisherBanner::GetRecord(
    const std::string& publisher_key,
    GetPublisherBannerCallback callback) {
  if (publisher_key.empty()) {
    engine_->Log(FROM_HERE) << "Publisher key is empty";
    std::move(callback).Run(nullptr);
    return;
  }
  auto transaction = mojom::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT title, description, background, logo, web3_url "
      "FROM %s "
      "WHERE publisher_key=?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseServerPublisherBanner::OnGetRecord,
                     base::Unretained(this), std::move(callback),
                     publisher_key));
}

void DatabaseServerPublisherBanner::OnGetRecord(
    GetPublisherBannerCallback callback,
    const std::string& publisher_key,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run(nullptr);
    return;
  }

  if (response->result->get_records().empty()) {
    engine_->Log(FROM_HERE) << "Server publisher banner not found";
    std::move(callback).Run(nullptr);
    return;
  }

  if (response->result->get_records().size() > 1) {
    engine_->Log(FROM_HERE) << "Record size is not correct: "
                            << response->result->get_records().size();
  }

  auto* record = response->result->get_records()[0].get();

  mojom::PublisherBanner banner;
  banner.publisher_key = publisher_key;
  banner.title = GetStringColumn(record, 0);
  banner.description = GetStringColumn(record, 1);
  banner.background = GetStringColumn(record, 2);
  banner.logo = GetStringColumn(record, 3);
  banner.web3_url = GetStringColumn(record, 4);

  // Get links
  links_.GetRecord(
      publisher_key,
      base::BindOnce(&DatabaseServerPublisherBanner::OnGetRecordLinks,
                     weak_factory_.GetWeakPtr(), banner, std::move(callback)));
}

void DatabaseServerPublisherBanner::OnGetRecordLinks(
    const mojom::PublisherBanner& banner,
    GetPublisherBannerCallback callback,
    const std::map<std::string, std::string>& links) {
  auto banner_pointer = mojom::PublisherBanner::New(banner);

  for (const auto& link : links) {
    banner_pointer->links.insert(link);
  }

  std::move(callback).Run(std::move(banner_pointer));
}

}  // namespace brave_rewards::internal::database

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_server_publisher_banner.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "server_publisher_banner";

}  // namespace

namespace braveledger_database {

DatabaseServerPublisherBanner::DatabaseServerPublisherBanner(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger),
    links_(std::make_unique<DatabaseServerPublisherLinks>(ledger)),
    amounts_(std::make_unique<DatabaseServerPublisherAmounts>(ledger)) {
}

DatabaseServerPublisherBanner::~DatabaseServerPublisherBanner() = default;

void DatabaseServerPublisherBanner::InsertOrUpdate(
    ledger::DBTransaction* transaction,
    const ledger::ServerPublisherInfo& server_info) {
  DCHECK(transaction);
  DCHECK(!server_info.publisher_key.empty());

  // Do not insert a record if there is no banner data
  // or if banner data is empty.
  ledger::PublisherBanner default_banner;
  if (!server_info.banner || server_info.banner->Equals(default_banner)) {
    BLOG(1, "Empty publisher banner data, skipping insert");
    return;
  }

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_key, title, description, background, logo) "
      "VALUES (?, ?, ?, ?, ?)",
      kTableName);

  BindString(command.get(), 0, server_info.publisher_key);
  BindString(command.get(), 1, server_info.banner->title);
  BindString(command.get(), 2, server_info.banner->description);
  BindString(command.get(), 3, server_info.banner->background);
  BindString(command.get(), 4, server_info.banner->logo);

  transaction->commands.push_back(std::move(command));

  links_->InsertOrUpdate(transaction, server_info);
  amounts_->InsertOrUpdate(transaction, server_info);
}

void DatabaseServerPublisherBanner::DeleteRecords(
    ledger::DBTransaction* transaction,
    const std::string& publisher_key_list) {
  DCHECK(transaction);
  if (publisher_key_list.empty()) {
    return;
  }

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "DELETE FROM %s WHERE publisher_key IN (%s)",
      kTableName,
      publisher_key_list.c_str());

  transaction->commands.push_back(std::move(command));

  links_->DeleteRecords(transaction, publisher_key_list);
  amounts_->DeleteRecords(transaction, publisher_key_list);
}

void DatabaseServerPublisherBanner::GetRecord(
    const std::string& publisher_key,
    ledger::PublisherBannerCallback callback) {
  if (publisher_key.empty()) {
    BLOG(1, "Publisher key is empty");
    callback(nullptr);
    return;
  }
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT title, description, background, logo "
      "FROM %s "
      "WHERE publisher_key=?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseServerPublisherBanner::OnGetRecord,
          this,
          _1,
          publisher_key,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseServerPublisherBanner::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    const std::string& publisher_key,
    ledger::PublisherBannerCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(nullptr);
    return;
  }

  if (response->result->get_records().empty()) {
    BLOG(1, "Server publisher banner not found");
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() > 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
  }

  auto* record = response->result->get_records()[0].get();

  ledger::PublisherBanner banner;
  banner.publisher_key = publisher_key;
  banner.title = GetStringColumn(record, 0);
  banner.description = GetStringColumn(record, 1);
  banner.background = GetStringColumn(record, 2);
  banner.logo = GetStringColumn(record, 3);

  // Get links
  auto links_callback =
      std::bind(&DatabaseServerPublisherBanner::OnGetRecordLinks,
          this,
          _1,
          banner,
          callback);
  links_->GetRecord(publisher_key, links_callback);
}

void DatabaseServerPublisherBanner::OnGetRecordLinks(
    const std::map<std::string, std::string>& links,
    const ledger::PublisherBanner& banner,
    ledger::PublisherBannerCallback callback) {
  auto banner_new = banner;

  for (auto& link : links) {
    banner_new.links.insert(link);
  }

  // Get amounts
  auto amounts_callback =
      std::bind(&DatabaseServerPublisherBanner::OnGetRecordAmounts,
          this,
          _1,
          banner_new,
          callback);
  amounts_->GetRecord(banner_new.publisher_key, amounts_callback);
}

void DatabaseServerPublisherBanner::OnGetRecordAmounts(
    const std::vector<double>& amounts,
    const ledger::PublisherBanner& banner,
    ledger::PublisherBannerCallback callback) {
  auto banner_pointer = ledger::PublisherBanner::New(banner);
  banner_pointer->amounts = amounts;
  callback(std::move(banner_pointer));
}

}  // namespace braveledger_database

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

const char table_name_[] = "server_publisher_banner";

}  // namespace

namespace braveledger_database {

DatabaseServerPublisherBanner::DatabaseServerPublisherBanner(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger),
    links_(std::make_unique<DatabaseServerPublisherLinks>(ledger)),
    amounts_(std::make_unique<DatabaseServerPublisherAmounts>(ledger)) {
}

DatabaseServerPublisherBanner::~DatabaseServerPublisherBanner() = default;

bool DatabaseServerPublisherBanner::CreateTableV7(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
      "title TEXT,"
      "description TEXT,"
      "background TEXT,"
      "logo TEXT,"
      "CONSTRAINT fk_%s_publisher_key"
      "    FOREIGN KEY (publisher_key)"
      "    REFERENCES server_publisher_info (publisher_key)"
      "    ON DELETE CASCADE"
      ")",
      table_name_,
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseServerPublisherBanner::CreateTableV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
      "publisher_key LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
      "title TEXT,"
      "description TEXT,"
      "background TEXT,"
      "logo TEXT"
      ")",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseServerPublisherBanner::CreateIndexV7(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "publisher_key");
}

bool DatabaseServerPublisherBanner::CreateIndexV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, table_name_, "publisher_key");
}

bool DatabaseServerPublisherBanner::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 7: {
      return MigrateToV7(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    default: {
      return true;
    }
  }
}


bool DatabaseServerPublisherBanner::MigrateToV7(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, table_name_)) {
    return false;
  }

  if (!CreateTableV7(transaction)) {
    return false;
  }

  if (!CreateIndexV7(transaction)) {
    return false;
  }

  if (!links_->Migrate(transaction, 7)) {
    return false;
  }

  if (!amounts_->Migrate(transaction, 7)) {
    return false;
  }

  return true;
}

bool DatabaseServerPublisherBanner::MigrateToV15(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name = base::StringPrintf(
      "%s_temp",
      table_name_);

  if (!RenameDBTable(transaction, table_name_, temp_table_name)) {
    return false;
  }

  const std::string query =
      "DROP INDEX IF EXISTS server_publisher_banner_publisher_key_index;";
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  if (!CreateTableV15(transaction)) {
    return false;
  }

  if (!CreateIndexV15(transaction)) {
    return false;
  }

  const std::map<std::string, std::string> columns = {
    { "publisher_key", "publisher_key" },
    { "title", "title" },
    { "description", "description" },
    { "background", "background" },
    { "logo", "logo" }
  };

  if (!MigrateDBTable(
      transaction,
      temp_table_name,
      table_name_,
      columns,
      true)) {
    return false;
  }

  if (!links_->Migrate(transaction, 15)) {
    return false;
  }

  if (!amounts_->Migrate(transaction, 15)) {
    return false;
  }

  return true;
}

bool DatabaseServerPublisherBanner::InsertOrUpdate(
    ledger::DBTransaction* transaction,
    ledger::ServerPublisherInfoPtr info) {
  DCHECK(transaction);

  if (!info || !info->banner) {
    return false;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_key, title, description, background, logo) "
      "VALUES (?, ?, ?, ?, ?)",
      table_name_);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->publisher_key);
  BindString(command.get(), 1, info->banner->title);
  BindString(command.get(), 2, info->banner->description);
  BindString(command.get(), 3, info->banner->background);
  BindString(command.get(), 4, info->banner->logo);

  transaction->commands.push_back(std::move(command));

  links_->InsertOrUpdate(transaction, info->Clone());
  amounts_->InsertOrUpdate(transaction, info->Clone());

  return true;
}

void DatabaseServerPublisherBanner::GetRecord(
    const std::string& publisher_key,
    ledger::PublisherBannerCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT title, description, background, logo "
      "FROM %s "
      "WHERE publisher_key=?",
      table_name_);

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

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseServerPublisherBanner::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    const std::string& publisher_key,
    ledger::PublisherBannerCallback callback) {
  if (!response
      || response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(nullptr);
    return;
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

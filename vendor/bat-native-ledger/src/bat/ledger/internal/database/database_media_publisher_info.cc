/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_media_publisher_info.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace ledger {
namespace database {

namespace {

const char kTableName[] = "media_publisher_info";

}  // namespace

DatabaseMediaPublisherInfo::DatabaseMediaPublisherInfo(
    LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseMediaPublisherInfo::~DatabaseMediaPublisherInfo() = default;

void DatabaseMediaPublisherInfo::InsertOrUpdate(
    const std::string& media_key,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  if (media_key.empty() || publisher_key.empty()) {
    BLOG(1, "Data is empty " << media_key << "/" << publisher_key);
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s (media_key, publisher_id) VALUES (?, ?)",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, media_key);
  BindString(command.get(), 1, publisher_key);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseMediaPublisherInfo::GetRecord(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  if (media_key.empty()) {
    BLOG(1, "Media key is empty");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  auto transaction = type::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "pi.provider, spi.status, spi.updated_at, pi.excluded "
      "FROM %s as mpi "
      "INNER JOIN publisher_info AS pi ON mpi.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE mpi.media_key=?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, media_key);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE,
      type::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseMediaPublisherInfo::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseMediaPublisherInfo::OnGetRecord(
    type::DBCommandResponsePtr response,
    ledger::PublisherInfoCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(1, "Response is wrong");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
    callback(type::Result::NOT_FOUND, {});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = type::PublisherInfo::New();

  info->id = GetStringColumn(record, 0);
  info->name = GetStringColumn(record, 1);
  info->url = GetStringColumn(record, 2);
  info->favicon_url = GetStringColumn(record, 3);
  info->provider = GetStringColumn(record, 4);
  info->status =
      static_cast<type::PublisherStatus>(GetIntColumn(record, 5));
  info->status_updated_at = GetInt64Column(record, 6);
  info->excluded =
      static_cast<type::PublisherExclude>(GetIntColumn(record, 7));

  callback(type::Result::LEDGER_OK, std::move(info));
}

}  // namespace database
}  // namespace ledger

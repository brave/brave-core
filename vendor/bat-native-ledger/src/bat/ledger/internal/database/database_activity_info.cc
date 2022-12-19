/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_activity_info.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "activity_info";

std::string GenerateActivityFilterQuery(
    const int start,
    const int limit,
    ledger::mojom::ActivityInfoFilterPtr filter) {
  std::string query = "";
  if (!filter) {
    return query;
  }

  if (!filter->id.empty()) {
    query += " AND ai.publisher_id = ?";
  }

  if (filter->reconcile_stamp > 0) {
    query += " AND ai.reconcile_stamp = ?";
  }

  if (filter->min_duration > 0) {
    query += " AND ai.duration >= ?";
  }

  if (filter->excluded != ledger::mojom::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
          ledger::mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded = ?";
  }

  if (filter->excluded ==
      ledger::mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded != ?";
  }

  if (filter->percent > 0) {
    query += " AND ai.percent >= ?";
  }

  if (filter->min_visits > 0) {
    query += " AND ai.visits >= ?";
  }

  if (!filter->non_verified) {
    const std::string status = base::StringPrintf(
        " AND spi.status != %1d", ledger::mojom::PublisherStatus::NOT_VERIFIED);
    query += status;
  }

  for (const auto& it : filter->order_by) {
    query += " ORDER BY " + it->property_name;
    query += (it->ascending ? " ASC" : " DESC");
  }

  if (limit > 0) {
    query += " LIMIT " + std::to_string(limit);

    if (start > 1) {
      query += " OFFSET " + std::to_string(start);
    }
  }

  return query;
}

void GenerateActivityFilterBind(ledger::mojom::DBCommand* command,
                                ledger::mojom::ActivityInfoFilterPtr filter) {
  if (!command || !filter) {
    return;
  }

  int column = 0;
  if (!filter->id.empty()) {
    ledger::database::BindString(command, column++, filter->id);
  }

  if (filter->reconcile_stamp > 0) {
    ledger::database::BindInt64(command, column++, filter->reconcile_stamp);
  }

  if (filter->min_duration > 0) {
    ledger::database::BindInt(command, column++, filter->min_duration);
  }

  if (filter->excluded != ledger::mojom::ExcludeFilter::FILTER_ALL &&
      filter->excluded !=
          ledger::mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    ledger::database::BindInt(
        command,
        column++,
        static_cast<int32_t>(filter->excluded));
  }

  if (filter->excluded ==
      ledger::mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    ledger::database::BindInt(
        command, column++,
        static_cast<int>(ledger::mojom::PublisherExclude::EXCLUDED));
  }

  if (filter->percent > 0) {
    ledger::database::BindInt(command, column++, filter->percent);
  }

  if (filter->min_visits > 0) {
    ledger::database::BindInt(command, column++, filter->min_visits);
  }
}

}  // namespace

namespace ledger {
namespace database {

DatabaseActivityInfo::DatabaseActivityInfo(
    LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseActivityInfo::~DatabaseActivityInfo() = default;

void DatabaseActivityInfo::NormalizeList(
    std::vector<mojom::PublisherInfoPtr> list,
    ledger::LegacyResultCallback callback) {
  if (list.empty()) {
    callback(mojom::Result::LEDGER_OK);
    return;
  }
  std::string main_query;
  for (const auto& info : list) {
    main_query += base::StringPrintf(
        "UPDATE %s SET percent = %d, weight = %f WHERE publisher_id = '%s';",
        kTableName, info->percent, info->weight, info->id.c_str());
  }

  if (main_query.empty()) {
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = mojom::DBTransaction::New();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = main_query;

  transaction->commands.push_back(std::move(command));

  auto shared_list =
      std::make_shared<std::vector<mojom::PublisherInfoPtr>>(std::move(list));

  ledger_->RunDBTransaction(
      std::move(transaction),
      [this, shared_list, callback](mojom::DBCommandResponsePtr response) {
        if (!response ||
            response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
          callback(mojom::Result::LEDGER_ERROR);
          return;
        }

        ledger_->ledger_client()->PublisherListNormalized(
            std::move(*shared_list));

        callback(mojom::Result::LEDGER_OK);
      });
}

void DatabaseActivityInfo::InsertOrUpdate(
    mojom::PublisherInfoPtr info,
    ledger::LegacyResultCallback callback) {
  if (!info) {
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = mojom::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, duration, score, percent, "
      "weight, reconcile_stamp, visits) "
      "VALUES (?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt64(command.get(), 1, static_cast<int>(info->duration));
  BindDouble(command.get(), 2, info->score);
  BindInt64(command.get(), 3, static_cast<int>(info->percent));
  BindDouble(command.get(), 4, info->weight);
  BindInt64(command.get(), 5, info->reconcile_stamp);
  BindInt(command.get(), 6, info->visits);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseActivityInfo::GetRecordsList(
    const int start,
    const int limit,
    mojom::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  if (!filter) {
    callback({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  std::string query = base::StringPrintf(
    "SELECT ai.publisher_id, ai.duration, ai.score, "
    "ai.percent, ai.weight, spi.status, spi.updated_at, pi.excluded, "
    "pi.name, pi.url, pi.provider, "
    "pi.favIcon, ai.reconcile_stamp, ai.visits "
    "FROM %s AS ai "
    "INNER JOIN publisher_info AS pi "
    "ON ai.publisher_id = pi.publisher_id "
    "LEFT JOIN server_publisher_info AS spi "
    "ON spi.publisher_key = pi.publisher_id "
    "WHERE 1 = 1",
    kTableName);

  query += GenerateActivityFilterQuery(start, limit, filter->Clone());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  GenerateActivityFilterBind(command.get(), filter->Clone());

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&DatabaseActivityInfo::OnGetRecordsList,
      this,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseActivityInfo::OnGetRecordsList(
    mojom::DBCommandResponsePtr response,
    ledger::PublisherInfoListCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    callback({});
    return;
  }

  std::vector<mojom::PublisherInfoPtr> list;
  for (auto const& record : response->result->get_records()) {
    auto info = mojom::PublisherInfo::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->duration = GetInt64Column(record_pointer, 1);
    info->score = GetDoubleColumn(record_pointer, 2);
    info->percent = GetInt64Column(record_pointer, 3);
    info->weight = GetDoubleColumn(record_pointer, 4);
    info->status =
        static_cast<mojom::PublisherStatus>(GetIntColumn(record_pointer, 5));
    info->status_updated_at = GetInt64Column(record_pointer, 6);
    info->excluded =
        static_cast<mojom::PublisherExclude>(GetIntColumn(record_pointer, 7));
    info->name = GetStringColumn(record_pointer, 8);
    info->url = GetStringColumn(record_pointer, 9);
    info->provider = GetStringColumn(record_pointer, 10);
    info->favicon_url = GetStringColumn(record_pointer, 11);
    info->reconcile_stamp = GetInt64Column(record_pointer, 12);
    info->visits = GetIntColumn(record_pointer, 13);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseActivityInfo::DeleteRecord(const std::string& publisher_key,
                                        ledger::LegacyResultCallback callback) {
  if (publisher_key.empty()) {
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE publisher_id = ? AND reconcile_stamp = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, publisher_key);
  BindInt64(command.get(), 1, ledger_->state()->GetReconcileStamp());

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseActivityInfo::GetPublishersVisitedCount(
    base::OnceCallback<void(int)> callback) {
  auto transaction = mojom::DBTransaction::New();

  std::string query = base::StringPrintf(
      "SELECT COUNT(DISTINCT ai.publisher_id) "
      "FROM %s AS ai INNER JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = ai.publisher_id "
      "WHERE ai.reconcile_stamp = ? AND spi.status > 1",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;
  BindInt64(command.get(), 0, ledger_->state()->GetReconcileStamp());
  command->record_bindings = {mojom::DBCommand::RecordBindingType::INT_TYPE};
  transaction->commands.push_back(std::move(command));

  auto on_read = [](base::OnceCallback<void(int)> callback,
                    mojom::DBCommandResponsePtr response) {
    if (!response ||
        response->status != mojom::DBCommandResponse::Status::RESPONSE_OK ||
        response->result->get_records().size() != 1) {
      std::move(callback).Run(0);
      return;
    }

    auto* record = response->result->get_records()[0].get();
    std::move(callback).Run(GetIntColumn(record, 0));
  };

  ledger_->RunDBTransaction(std::move(transaction),
                            base::BindOnce(on_read, std::move(callback)));
}

}  // namespace database
}  // namespace ledger

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_rewards/core/database/database_activity_info.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/state/state.h"

namespace brave_rewards::internal {

namespace {

constexpr char kTableName[] = "activity_info";

std::string GenerateActivityFilterQuery(const int start,
                                        const int limit,
                                        mojom::ActivityInfoFilterPtr filter) {
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

  if (filter->excluded != mojom::ExcludeFilter::FILTER_ALL &&
      filter->excluded != mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    query += " AND pi.excluded = ?";
  }

  if (filter->excluded == mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
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
        " AND spi.status != %1d AND spi.address != ''",
        base::to_underlying(mojom::PublisherStatus::NOT_VERIFIED));
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

void GenerateActivityFilterBind(mojom::DBCommand* command,
                                mojom::ActivityInfoFilterPtr filter) {
  if (!command || !filter) {
    return;
  }

  int column = 0;
  if (!filter->id.empty()) {
    database::BindString(command, column++, filter->id);
  }

  if (filter->reconcile_stamp > 0) {
    database::BindInt64(command, column++, filter->reconcile_stamp);
  }

  if (filter->min_duration > 0) {
    database::BindInt(command, column++, filter->min_duration);
  }

  if (filter->excluded != mojom::ExcludeFilter::FILTER_ALL &&
      filter->excluded != mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    database::BindInt(command, column++,
                      static_cast<int32_t>(filter->excluded));
  }

  if (filter->excluded == mojom::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED) {
    database::BindInt(command, column++,
                      static_cast<int>(mojom::PublisherExclude::EXCLUDED));
  }

  if (filter->percent > 0) {
    database::BindInt(command, column++, filter->percent);
  }

  if (filter->min_visits > 0) {
    database::BindInt(command, column++, filter->min_visits);
  }
}

}  // namespace

namespace database {

DatabaseActivityInfo::DatabaseActivityInfo(RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabaseActivityInfo::~DatabaseActivityInfo() = default;

void DatabaseActivityInfo::NormalizeList(
    std::vector<mojom::PublisherInfoPtr> list,
    ResultCallback callback) {
  if (list.empty()) {
    std::move(callback).Run(mojom::Result::OK);
    return;
  }
  std::string main_query;
  for (const auto& info : list) {
    main_query += base::StringPrintf(
        "UPDATE %s SET percent = %d, weight = %f WHERE publisher_id = '%s';",
        kTableName, info->percent, info->weight, info->id.c_str());
  }

  if (main_query.empty()) {
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = main_query;

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseActivityInfo::OnNormalizeList,
                     base::Unretained(this), std::move(callback),
                     std::move(list)));
}

void DatabaseActivityInfo::OnNormalizeList(
    ResultCallback callback,
    std::vector<mojom::PublisherInfoPtr> list,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  engine_->client()->PublisherListNormalized(std::move(list));

  std::move(callback).Run(mojom::Result::OK);
}

void DatabaseActivityInfo::InsertOrUpdate(mojom::PublisherInfoPtr info,
                                          ResultCallback callback) {
  if (!info) {
    std::move(callback).Run(mojom::Result::FAILED);
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

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseActivityInfo::GetRecordsList(
    const int start,
    const int limit,
    mojom::ActivityInfoFilterPtr filter,
    GetActivityInfoListCallback callback) {
  if (!filter) {
    std::move(callback).Run({});
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

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseActivityInfo::OnGetRecordsList,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseActivityInfo::OnGetRecordsList(
    GetActivityInfoListCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    std::move(callback).Run({});
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
    info->status = PublisherStatusFromInt(GetIntColumn(record_pointer, 5));
    info->status_updated_at = GetInt64Column(record_pointer, 6);
    info->excluded = PublisherExcludeFromInt(GetIntColumn(record_pointer, 7));
    info->name = GetStringColumn(record_pointer, 8);
    info->url = GetStringColumn(record_pointer, 9);
    info->provider = GetStringColumn(record_pointer, 10);
    info->favicon_url = GetStringColumn(record_pointer, 11);
    info->reconcile_stamp = GetInt64Column(record_pointer, 12);
    info->visits = GetIntColumn(record_pointer, 13);

    list.push_back(std::move(info));
  }

  std::move(callback).Run(std::move(list));
}

void DatabaseActivityInfo::DeleteRecord(const std::string& publisher_key,
                                        ResultCallback callback) {
  if (publisher_key.empty()) {
    std::move(callback).Run(mojom::Result::FAILED);
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
  BindInt64(command.get(), 1, engine_->state()->GetReconcileStamp());

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseActivityInfo::GetPublishersVisitedCount(
    base::OnceCallback<void(int)> callback) {
  auto transaction = mojom::DBTransaction::New();

  std::string query = base::StringPrintf(
      "SELECT COUNT(DISTINCT ai.publisher_id) "
      "FROM %s AS ai INNER JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = ai.publisher_id "
      "WHERE ai.reconcile_stamp = ? AND spi.status > 1 AND spi.address != ''",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;
  BindInt64(command.get(), 0, engine_->state()->GetReconcileStamp());
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

  engine_->client()->RunDBTransaction(
      std::move(transaction), base::BindOnce(on_read, std::move(callback)));
}

}  // namespace database
}  // namespace brave_rewards::internal

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_rewards/core/database/database_balance_report.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {

namespace {

const char kTableName[] = "balance_report_info";

std::string GetBalanceReportId(mojom::ActivityMonth month, int year) {
  return base::StringPrintf("%u_%u", year, base::to_underlying(month));
}

std::string GetTypeColumn(mojom::ReportType type) {
  switch (type) {
    case mojom::ReportType::GRANT_UGP: {
      return "grants_ugp";
    }
    case mojom::ReportType::GRANT_AD: {
      return "grants_ads";
    }
    case mojom::ReportType::AUTO_CONTRIBUTION: {
      return "auto_contribute";
    }
    case mojom::ReportType::TIP: {
      return "tip";
    }
    case mojom::ReportType::TIP_RECURRING: {
      return "tip_recurring";
    }
  }
}

}  // namespace

namespace database {

DatabaseBalanceReport::DatabaseBalanceReport(RewardsEngineImpl& engine)
    : DatabaseTable(engine) {}

DatabaseBalanceReport::~DatabaseBalanceReport() = default;

void DatabaseBalanceReport::InsertOrUpdate(mojom::BalanceReportInfoPtr info,
                                           LegacyResultCallback callback) {
  if (!info || info->id.empty()) {
    BLOG(1, "Id is empty");
    callback(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
      "tip_recurring, tip) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindDouble(command.get(), 1, info->grants);
  BindDouble(command.get(), 2, info->earning_from_ads);
  BindDouble(command.get(), 3, info->auto_contribute);
  BindDouble(command.get(), 4, info->recurring_donation);
  BindDouble(command.get(), 5, info->one_time_donation);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseBalanceReport::InsertOrUpdateList(
    std::vector<mojom::BalanceReportInfoPtr> list,
    LegacyResultCallback callback) {
  if (list.empty()) {
    BLOG(1, "List is empty");
    callback(mojom::Result::OK);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
      "tip_recurring, tip) "
      "VALUES (?, ?, ?, ?, ?, ?);",
      kTableName);

  for (const auto& report : list) {
    auto command = mojom::DBCommand::New();
    command->type = mojom::DBCommand::Type::RUN;
    command->command = query;

    BindString(command.get(), 0, report->id);
    BindDouble(command.get(), 1, report->grants);
    BindDouble(command.get(), 2, report->earning_from_ads);
    BindDouble(command.get(), 3, report->auto_contribute);
    BindDouble(command.get(), 4, report->recurring_donation);
    BindDouble(command.get(), 5, report->one_time_donation);

    transaction->commands.push_back(std::move(command));
  }

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseBalanceReport::SetAmount(mojom::ActivityMonth month,
                                      int year,
                                      mojom::ReportType type,
                                      double amount,
                                      LegacyResultCallback callback) {
  if (month == mojom::ActivityMonth::ANY || year == 0) {
    BLOG(1, "Record size is not correct " << month << "/" << year);
    callback(mojom::Result::FAILED);
    return;
  }
  auto transaction = mojom::DBTransaction::New();

  const std::string id = GetBalanceReportId(month, year);

  const std::string insert_query = base::StringPrintf(
      "INSERT OR IGNORE INTO %s "
      "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
      "tip_recurring, tip) "
      "VALUES (?, 0, 0, 0, 0, 0) ",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = insert_query;
  BindString(command.get(), 0, id);
  transaction->commands.push_back(std::move(command));

  const std::string update_query = base::StringPrintf(
      "UPDATE %s SET %s = %s + ? WHERE balance_report_id = ?", kTableName,
      GetTypeColumn(type).c_str(), GetTypeColumn(type).c_str());

  command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = update_query;
  BindDouble(command.get(), 0, amount);
  BindString(command.get(), 1, id);
  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseBalanceReport::GetRecord(
    mojom::ActivityMonth month,
    int year,
    mojom::RewardsEngine::GetBalanceReportCallback callback) {
  if (month == mojom::ActivityMonth::ANY || year == 0) {
    BLOG(1, "Record size is not correct " << month << "/" << year);
    return std::move(callback).Run(mojom::Result::FAILED, {});
  }

  auto transaction = mojom::DBTransaction::New();

  // when new month starts we need to insert blank values
  const std::string insert_query = base::StringPrintf(
      "INSERT OR IGNORE INTO %s "
      "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
      "tip_recurring, tip) "
      "VALUES (?, 0, 0, 0, 0, 0) ",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = insert_query;
  BindString(command.get(), 0, GetBalanceReportId(month, year));
  transaction->commands.push_back(std::move(command));

  const std::string select_query = base::StringPrintf(
      "SELECT balance_report_id, grants_ugp, grants_ads, "
      "auto_contribute, tip_recurring, tip "
      "FROM %s WHERE balance_report_id = ?",
      kTableName);

  command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = select_query;

  BindString(command.get(), 0, GetBalanceReportId(month, year));

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseBalanceReport::OnGetRecord,
                     base::Unretained(this), std::move(callback)));
}
void DatabaseBalanceReport::OnGetRecord(
    mojom::RewardsEngine::GetBalanceReportCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    return std::move(callback).Run(mojom::Result::FAILED, {});
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: "
                << response->result->get_records().size());
    return std::move(callback).Run(mojom::Result::FAILED, {});
  }

  auto* record = response->result->get_records()[0].get();

  auto info = mojom::BalanceReportInfo::New();
  info->id = GetStringColumn(record, 0);
  info->grants = GetDoubleColumn(record, 1);
  info->earning_from_ads = GetDoubleColumn(record, 2);
  info->auto_contribute = GetDoubleColumn(record, 3);
  info->recurring_donation = GetDoubleColumn(record, 4);
  info->one_time_donation = GetDoubleColumn(record, 5);

  std::move(callback).Run(mojom::Result::OK, std::move(info));
}

void DatabaseBalanceReport::GetAllRecords(
    GetBalanceReportListCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT balance_report_id, grants_ugp, grants_ads, "
      "auto_contribute, tip_recurring, tip "
      "FROM %s",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseBalanceReport::OnGetAllRecords,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseBalanceReport::OnGetAllRecords(
    GetBalanceReportListCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  std::vector<mojom::BalanceReportInfoPtr> list;
  for (const auto& record : response->result->get_records()) {
    auto* record_pointer = record.get();
    auto info = mojom::BalanceReportInfo::New();

    info->id = GetStringColumn(record_pointer, 0);
    info->grants = GetDoubleColumn(record_pointer, 1);
    info->earning_from_ads = GetDoubleColumn(record_pointer, 2);
    info->auto_contribute = GetDoubleColumn(record_pointer, 3);
    info->recurring_donation = GetDoubleColumn(record_pointer, 4);
    info->one_time_donation = GetDoubleColumn(record_pointer, 5);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseBalanceReport::DeleteAllRecords(LegacyResultCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf("DELETE FROM %s", kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

}  // namespace database
}  // namespace brave_rewards::internal

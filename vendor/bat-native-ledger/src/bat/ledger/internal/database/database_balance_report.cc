/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/database/database_balance_report.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "balance_report_info";

std::string GetBalanceReportId(
    ledger::ActivityMonth month,
    int year) {
  return base::StringPrintf("%u_%u", year, month);
}

std::string GetTypeColumn(ledger::ReportType type) {
  switch (type) {
    case ledger::ReportType::GRANT_UGP: {
      return "grants_ugp";
    }
    case ledger::ReportType::GRANT_AD: {
      return "grants_ads";
    }
    case ledger::ReportType::AUTO_CONTRIBUTION: {
      return "auto_contribute";
    }
    case ledger::ReportType::TIP: {
      return "tip";
    }
    case ledger::ReportType::TIP_RECURRING: {
      return "tip_recurring";
    }
  }
}

}  // namespace

namespace braveledger_database {

DatabaseBalanceReport::DatabaseBalanceReport(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseBalanceReport::~DatabaseBalanceReport() = default;

void DatabaseBalanceReport::InsertOrUpdate(
    ledger::BalanceReportInfoPtr info,
    ledger::ResultCallback callback) {
  if (!info || info->id.empty()) {
    BLOG(1, "Id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
      "tip_recurring, tip) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindDouble(command.get(), 1, info->grants);
  BindDouble(command.get(), 2, info->earning_from_ads);
  BindDouble(command.get(), 3, info->auto_contribute);
  BindDouble(command.get(), 4, info->recurring_donation);
  BindDouble(command.get(), 5, info->one_time_donation);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseBalanceReport::InsertOrUpdateList(
    ledger::BalanceReportInfoList list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    BLOG(1, "List is empty");
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
      "tip_recurring, tip) "
      "VALUES (?, ?, ?, ?, ?, ?);",
      kTableName);

  for (const auto& report : list) {
    auto command = ledger::DBCommand::New();
    command->type = ledger::DBCommand::Type::RUN;
    command->command = query;

    BindString(command.get(), 0, report->id);
    BindDouble(command.get(), 1, report->grants);
    BindDouble(command.get(), 2, report->earning_from_ads);
    BindDouble(command.get(), 3, report->auto_contribute);
    BindDouble(command.get(), 4, report->recurring_donation);
    BindDouble(command.get(), 5, report->one_time_donation);

    transaction->commands.push_back(std::move(command));
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseBalanceReport::SetAmount(
    ledger::ActivityMonth month,
    int year,
    ledger::ReportType type,
    double amount,
    ledger::ResultCallback callback) {
  if (month == ledger::ActivityMonth::ANY || year == 0) {
    BLOG(1, "Record size is not correct " << month << "/" << year);
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }
  auto transaction = ledger::DBTransaction::New();

  const std::string id = GetBalanceReportId(month, year);

  const std::string insert_query = base::StringPrintf(
      "INSERT OR IGNORE INTO %s "
      "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
      "tip_recurring, tip) "
      "VALUES (?, 0, 0, 0, 0, 0) ",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = insert_query;
  BindString(command.get(), 0, id);
  transaction->commands.push_back(std::move(command));

  const std::string update_query = base::StringPrintf(
      "UPDATE %s SET %s = %s + ? WHERE balance_report_id = ?",
      kTableName,
      GetTypeColumn(type).c_str(),
      GetTypeColumn(type).c_str());

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = update_query;
  BindDouble(command.get(), 0, amount);
  BindString(command.get(), 1, id);
  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseBalanceReport::GetRecord(
    ledger::ActivityMonth month,
    int year,
    ledger::GetBalanceReportCallback callback) {
  if (month == ledger::ActivityMonth::ANY || year == 0) {
    BLOG(1, "Record size is not correct " << month << "/" << year);
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  // when new month starts we need to insert blank values
  const std::string insert_query = base::StringPrintf(
      "INSERT OR IGNORE INTO %s "
      "(balance_report_id, grants_ugp, grants_ads, auto_contribute, "
      "tip_recurring, tip) "
      "VALUES (?, 0, 0, 0, 0, 0) ",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = insert_query;
  BindString(command.get(), 0, GetBalanceReportId(month, year));
  transaction->commands.push_back(std::move(command));

  const std::string select_query = base::StringPrintf(
    "SELECT balance_report_id, grants_ugp, grants_ads, "
    "auto_contribute, tip_recurring, tip "
    "FROM %s WHERE balance_report_id = ?",
    kTableName);

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = select_query;

  BindString(command.get(), 0, GetBalanceReportId(month, year));

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(
      &DatabaseBalanceReport::OnGetRecord,
      this,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}
void DatabaseBalanceReport::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ledger::GetBalanceReportCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = ledger::BalanceReportInfo::New();
  info->id = GetStringColumn(record, 0);
  info->grants = GetDoubleColumn(record, 1);
  info->earning_from_ads = GetDoubleColumn(record, 2);
  info->auto_contribute = GetDoubleColumn(record, 3);
  info->recurring_donation = GetDoubleColumn(record, 4);
  info->one_time_donation = GetDoubleColumn(record, 5);

  callback(ledger::Result::LEDGER_OK, std::move(info));
}

void DatabaseBalanceReport::GetAllRecords(
    ledger::GetBalanceReportListCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT balance_report_id, grants_ugp, grants_ads, "
    "auto_contribute, tip_recurring, tip "
    "FROM %s",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseBalanceReport::OnGetAllRecords,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseBalanceReport::OnGetAllRecords(
    ledger::DBCommandResponsePtr response,
    ledger::GetBalanceReportListCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  ledger::BalanceReportInfoList list;
  for (const auto& record : response->result->get_records()) {
    auto* record_pointer = record.get();
    auto info = ledger::BalanceReportInfo::New();

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

void DatabaseBalanceReport::DeleteAllRecords(
    ledger::ResultCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf("DELETE FROM %s", kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

}  // namespace braveledger_database

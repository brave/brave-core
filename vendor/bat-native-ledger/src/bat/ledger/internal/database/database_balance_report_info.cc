/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/database/database_balance_report_info.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "balance_report_info";

std::string GetBalanceReportName(
    ledger::ActivityMonth month,
    int year) {
  return base::StringPrintf("%u_%u", year, month);
}

}  // namespace

namespace braveledger_database {

DatabaseBalanceReportInfo::DatabaseBalanceReportInfo(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseBalanceReportInfo::~DatabaseBalanceReportInfo() = default;

bool DatabaseBalanceReportInfo::CreateTableV21(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "balance_report_id LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE,"
        "grants DOUBLE DEFAULT 0 NOT NULL,"
        "earning_from_ads DOUBLE DEFAULT 0 NOT NULL,"
        "auto_contribute DOUBLE DEFAULT 0 NOT NULL,"
        "recurring_donation DOUBLE DEFAULT 0 NOT NULL,"
        "one_time_donation DOUBLE DEFAULT 0 NOT NULL"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseBalanceReportInfo::CreateIndexV21(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "balance_report_id");
}

bool DatabaseBalanceReportInfo::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 21: {
      return MigrateToV21(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseBalanceReportInfo::MigrateToV21(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    return false;
  }

  if (!CreateTableV21(transaction)) {
    return false;
  }

  if (!CreateIndexV21(transaction)) {
    return false;
  }

  // FIXME: Migrate existing data from publisher state

  return true;
}

void DatabaseBalanceReportInfo::InsertOrUpdate(
    ledger::BalanceReportInfoPtr info,
    ledger::ResultCallback callback) {
  if (!info || info->id.empty()) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(balance_report_id, grants, earning_from_ads, auto_contribute, "
      "recurring_donation, one_time_donation) "
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

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseBalanceReportInfo::InsertOrUpdateItem(
    ledger::ActivityMonth month,
    int year,
    ledger::ReportType type,
    double amount,
    ledger::ResultCallback callback) {
  GetRecord(month, year,
            [this, type, amount, callback](ledger::Result result,
                                           ledger::BalanceReportInfoPtr info) {
              if (result != ledger::Result::LEDGER_OK) {
                callback(ledger::Result::LEDGER_ERROR);
                return;
              }
              UpdateItemAmount(std::move(info), type, amount, callback);
            });
}

void DatabaseBalanceReportInfo::GetRecord(
    ledger::ActivityMonth month,
    int year,
    ledger::GetBalanceReportCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT mb.balance_report_id, mb.grants, mb.earning_from_ads, "
    "mb.auto_contribute, mb.recurring_donation, mb.one_time_donation "
    "FROM %s as mb "
    "WHERE balance_report_id=?",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, GetBalanceReportName(month, year));

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
      &DatabaseBalanceReportInfo::OnGetRecord,
      this,
      _1,
      month,
      year,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseBalanceReportInfo::GetAllRecords(
    ledger::GetBalanceReportListCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT mb.balance_report_id, mb.grants, mb.earning_from_ads, "
    "mb.auto_contribute, mb.recurring_donation, mb.one_time_donation "
    "FROM %s as mb ",
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
      std::bind(&DatabaseBalanceReportInfo::OnGetAllRecords,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseBalanceReportInfo::DeleteAllRecords(
    ledger::ResultCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf("DELETE FROM %s", kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseBalanceReportInfo::UpdateItemAmount(
    ledger::BalanceReportInfoPtr info,
    ledger::ReportType type,
    double amount,
    ledger::ResultCallback callback) {
  switch (type) {
    case ledger::ReportType::GRANT_UGP:
      info->grants = info->grants + amount;
      break;
    case ledger::ReportType::GRANT_AD:
      info->earning_from_ads = info->earning_from_ads + amount;
      break;
    case ledger::ReportType::AUTO_CONTRIBUTION:
      info->auto_contribute = info->auto_contribute + amount;
      break;
    case ledger::ReportType::TIP:
      info->one_time_donation = info->one_time_donation + amount;
      break;
    case ledger::ReportType::TIP_RECURRING:
      info->recurring_donation = info->recurring_donation + amount;
      break;
    default:
      break;
  }

  InsertOrUpdate(std::move(info), callback);
}

void DatabaseBalanceReportInfo::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ledger::ActivityMonth month,
    int year,
    ledger::GetBalanceReportCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (response->result->get_records().size() != 1) {
    auto new_report_info = ledger::BalanceReportInfo::New();
    new_report_info->id = GetBalanceReportName(month, year);
    new_report_info->grants = 0.0;
    new_report_info->earning_from_ads = 0.0;
    new_report_info->auto_contribute = 0.0;
    new_report_info->recurring_donation = 0.0;
    new_report_info->one_time_donation = 0.0;

    auto insert_callback =
        std::bind(&DatabaseBalanceReportInfo::OnInsertOrUpdateInternal,
            this,
            _1,
            month,
            year,
            callback);

    InsertOrUpdate(std::move(new_report_info), insert_callback);
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

void DatabaseBalanceReportInfo::OnInsertOrUpdateInternal(
    ledger::Result result,
    ledger::ActivityMonth month,
    int year,
    ledger::GetBalanceReportCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  GetRecord(
      month, year,
      [callback](ledger::Result result, ledger::BalanceReportInfoPtr info) {
        if (result != ledger::Result::LEDGER_OK) {
          callback(ledger::Result::LEDGER_ERROR, {});
          return;
        }
        callback(ledger::Result::LEDGER_OK, std::move(info));
      });
}

void DatabaseBalanceReportInfo::OnGetAllRecords(
    ledger::DBCommandResponsePtr response,
    ledger::GetBalanceReportListCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
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

}  // namespace braveledger_database

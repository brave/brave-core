/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_contribution_info.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "contribution_info";
const char kChildTableName[] = "contribution_info_publishers";

ledger::ReportType ConvertRewardsTypeToReportType(
    const ledger::RewardsType type) {
  switch (type) {
    case ledger::RewardsType::AUTO_CONTRIBUTE: {
      return ledger::ReportType::AUTO_CONTRIBUTION;
    }
    case ledger::RewardsType::ONE_TIME_TIP: {
      return ledger::ReportType::TIP;
    }
    case ledger::RewardsType::RECURRING_TIP: {
      return ledger::ReportType::TIP_RECURRING;
    }
    default: {
      NOTREACHED();
      return ledger::ReportType::TIP;
    }
  }
}

}  // namespace

DatabaseContributionInfo::DatabaseContributionInfo(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger),
    publishers_(std::make_unique<DatabaseContributionInfoPublishers>(ledger)) {
}

DatabaseContributionInfo::~DatabaseContributionInfo() = default;

void DatabaseContributionInfo::InsertOrUpdate(
    ledger::ContributionInfoPtr info,
    ledger::ResultCallback callback) {
  if (!info) {
    BLOG(1, "Info is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto created_at = info->created_at;
  if (info->created_at == 0) {
    created_at = braveledger_time_util::GetCurrentTimeStamp();
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(contribution_id, amount, type, step, retry_count, created_at, "
      "processor) "
      "VALUES (?, ?, ?, ?, ?, ?, ?)",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->contribution_id);
  BindDouble(command.get(), 1, info->amount);
  BindInt(command.get(), 2, static_cast<int>(info->type));
  BindInt(command.get(), 3, static_cast<int>(info->step));
  BindInt(command.get(), 4, info->retry_count);
  BindInt64(command.get(), 5, created_at);
  BindInt(command.get(), 6, static_cast<int>(info->processor));

  transaction->commands.push_back(std::move(command));

  publishers_->InsertOrUpdate(transaction.get(), info->Clone());

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionInfo::GetRecord(
    const std::string& contribution_id,
    ledger::GetContributionInfoCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT ci.contribution_id, ci.amount, ci.type, ci.step, ci.retry_count, "
    "ci.processor "
    "FROM %s as ci "
    "WHERE ci.contribution_id = ?",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, contribution_id);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionInfo::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionInfo::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ledger::GetContributionInfoCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is not ok");
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(1, "Record size is not correct: " <<
        response->result->get_records().size());
    callback(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = ledger::ContributionInfo::New();
  info->contribution_id = GetStringColumn(record, 0);
  info->amount = GetDoubleColumn(record, 1);
  info->type = static_cast<ledger::RewardsType>(GetInt64Column(record, 2));
  info->step = static_cast<ledger::ContributionStep>(GetIntColumn(record, 3));
  info->retry_count = GetIntColumn(record, 4);
  info->processor =
      static_cast<ledger::ContributionProcessor>(GetIntColumn(record, 5));

  auto publishers_callback =
    std::bind(&DatabaseContributionInfo::OnGetPublishers,
        this,
        _1,
        braveledger_bind_util::FromContributionToString(info->Clone()),
        callback);

  publishers_->GetRecordByContributionList(
      {info->contribution_id},
      publishers_callback);
}

void DatabaseContributionInfo::OnGetPublishers(
    ledger::ContributionPublisherList list,
    const std::string& contribution_string,
    ledger::GetContributionInfoCallback callback) {

  auto contribution = braveledger_bind_util::FromStringToContribution(
      contribution_string);

  if (!contribution) {
    BLOG(1, "Contribution is null");
    callback(nullptr);
    return;
  }

  contribution->publishers = std::move(list);
  callback(std::move(contribution));
}

void DatabaseContributionInfo::GetAllRecords(
    ledger::ContributionInfoListCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "SELECT ci.contribution_id, ci.amount, ci.type, ci.step, ci.retry_count,"
    "ci.processor, ci.created_at "
    "FROM %s as ci ",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionInfo::OnGetList,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionInfo::GetOneTimeTips(
    const ledger::ActivityMonth month,
    const int year,
    ledger::PublisherInfoListCallback callback) {
  if (year == 0) {
    BLOG(1, "Year is 0");
    callback({});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "ci.amount, ci.created_at, spi.status, spi.updated_at, pi.provider "
      "FROM %s as ci "
      "INNER JOIN %s AS cp "
      "ON cp.contribution_id = ci.contribution_id "
      "INNER JOIN publisher_info AS pi ON cp.publisher_key = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE strftime('%%m',  datetime(ci.created_at, 'unixepoch')) = ? AND "
      "strftime('%%Y', datetime(ci.created_at, 'unixepoch')) = ? "
      "AND ci.type = ? AND ci.step = ?",
      kTableName,
      kChildTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  const std::string formatted_month = base::StringPrintf("%02d", month);

  BindString(command.get(), 0, formatted_month);
  BindString(command.get(), 1, std::to_string(year));
  BindInt(command.get(), 2,
      static_cast<int>(ledger::RewardsType::ONE_TIME_TIP));
  BindInt(command.get(), 3,
      static_cast<int>(ledger::ContributionStep::STEP_COMPLETED));

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionInfo::OnGetOneTimeTips,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionInfo::OnGetOneTimeTips(
    ledger::DBCommandResponsePtr response,
    ledger::PublisherInfoListCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is not ok");
    callback({});
    return;
  }

  ledger::PublisherInfoList list;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::PublisherInfo::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->name = GetStringColumn(record_pointer, 1);
    info->url = GetStringColumn(record_pointer, 2);
    info->favicon_url = GetStringColumn(record_pointer, 3);
    info->weight = GetDoubleColumn(record_pointer, 4);
    info->reconcile_stamp = GetInt64Column(record_pointer, 5);
    info->status = static_cast<ledger::mojom::PublisherStatus>(
        GetInt64Column(record_pointer, 6));
    info->status_updated_at = GetInt64Column(record_pointer, 7);
    info->provider = GetStringColumn(record_pointer, 8);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabaseContributionInfo::GetContributionReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetContributionReportCallback callback) {
  if (year == 0) {
    BLOG(1, "Year is 0");
    callback({});
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT ci.contribution_id, ci.amount, ci.type, ci.created_at, "
      "ci.processor FROM %s as ci "
      "WHERE strftime('%%m',  datetime(ci.created_at, 'unixepoch')) = ? AND "
      "strftime('%%Y', datetime(ci.created_at, 'unixepoch')) = ? AND step = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  const std::string formatted_month = base::StringPrintf("%02d", month);

  BindString(command.get(), 0, formatted_month);
  BindString(command.get(), 1, std::to_string(year));
  BindInt(command.get(), 2,
      static_cast<int>(ledger::ContributionStep::STEP_COMPLETED));

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionInfo::OnGetContributionReport,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionInfo::OnGetContributionReport(
    ledger::DBCommandResponsePtr response,
    ledger::GetContributionReportCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is not ok");
    callback({});
    return;
  }

  ledger::ContributionInfoList list;
  std::vector<std::string> contribution_ids;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::ContributionInfo::New();
    auto* record_pointer = record.get();

    info->contribution_id = GetStringColumn(record_pointer, 0);
    info->amount = GetDoubleColumn(record_pointer, 1);
    info->type = static_cast<ledger::RewardsType>(
        GetInt64Column(record_pointer, 2));
    info->created_at = GetInt64Column(record_pointer, 3);
    info->processor = static_cast<ledger::ContributionProcessor>(
        GetIntColumn(record_pointer, 4));

    contribution_ids.push_back(info->contribution_id);
    list.push_back(std::move(info));
  }

  auto publisher_callback =
      std::bind(&DatabaseContributionInfo::OnGetContributionReportPublishers,
          this,
          _1,
          braveledger_bind_util::FromContributionListToString(std::move(list)),
          callback);

  publishers_->GetContributionPublisherPairList(
      contribution_ids,
      publisher_callback);
}

void DatabaseContributionInfo::OnGetContributionReportPublishers(
    std::vector<ContributionPublisherInfoPair> publisher_pair_list,
    const std::string& contribution_list_string,
    ledger::GetContributionReportCallback callback) {
  ledger::ContributionInfoList contribution_list;
  braveledger_bind_util::FromStringToContributionList(
      contribution_list_string,
      &contribution_list);

  ledger::ContributionReportInfoList report_list;
  for (auto& contribution : contribution_list) {
    auto report = ledger::ContributionReportInfo::New();
    report->contribution_id = contribution->contribution_id;
    report->amount = contribution->amount;
    report->type = ConvertRewardsTypeToReportType(contribution->type);
    report->processor = contribution->processor;
    report->created_at = contribution->created_at;

    report_list.push_back(std::move(report));
  }

  for (auto& report : report_list) {
    for (auto& item : publisher_pair_list) {
      if (item.first != report->contribution_id) {
        continue;
      }

      report->publishers.push_back(std::move(item.second));
    }
  }

  callback(std::move(report_list));
}

void DatabaseContributionInfo::GetNotCompletedRecords(
    ledger::ContributionInfoListCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT ci.contribution_id, ci.amount, ci.type, ci.step, ci.retry_count, "
      "ci.processor, ci.created_at "
      "FROM %s as ci WHERE ci.step > 0",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionInfo::OnGetList,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionInfo::OnGetList(
    ledger::DBCommandResponsePtr response,
    ledger::ContributionInfoListCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is not ok");
    callback({});
    return;
  }

  if (response->result->get_records().empty()) {
    callback({});
    return;
  }

  ledger::ContributionInfoList list;
  std::vector<std::string> contribution_ids;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::ContributionInfo::New();
    auto* record_pointer = record.get();

    info->contribution_id = GetStringColumn(record_pointer, 0);
    info->amount = GetDoubleColumn(record_pointer, 1);
    info->type = static_cast<ledger::RewardsType>(
        GetInt64Column(record_pointer, 2));
    info->step = static_cast<ledger::ContributionStep>(
        GetIntColumn(record_pointer, 3));
    info->retry_count = GetIntColumn(record_pointer, 4);
    info->processor = static_cast<ledger::ContributionProcessor>(
        GetIntColumn(record_pointer, 5));
    info->created_at = GetInt64Column(record_pointer, 6);

    contribution_ids.push_back(info->contribution_id);
    list.push_back(std::move(info));
  }

  auto publisher_callback =
      std::bind(&DatabaseContributionInfo::OnGetListPublishers,
          this,
          _1,
          braveledger_bind_util::FromContributionListToString(std::move(list)),
          callback);

  publishers_->GetRecordByContributionList(
      contribution_ids,
      publisher_callback);
}

void DatabaseContributionInfo::OnGetListPublishers(
    ledger::ContributionPublisherList list,
    const std::string& contribution_list_string,
    ledger::ContributionInfoListCallback callback) {
  ledger::ContributionInfoList contribution_list;
  braveledger_bind_util::FromStringToContributionList(
      contribution_list_string,
      &contribution_list);

  for (auto& contribution : contribution_list) {
    for (auto& item : list) {
      if (item->contribution_id != contribution->contribution_id) {
        continue;
      }

      contribution->publishers.push_back(item->Clone());
    }
  }

  callback(std::move(contribution_list));
}

void DatabaseContributionInfo::UpdateStep(
    const std::string& contribution_id,
    const ledger::ContributionStep step,
    ledger::ResultCallback callback) {
  if (contribution_id.empty()) {
    BLOG(1, "Contribution id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "UPDATE %s SET step=?, retry_count=0 WHERE contribution_id = ?",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(step));
  BindString(command.get(), 1, contribution_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionInfo::UpdateStepAndCount(
    const std::string& contribution_id,
    const ledger::ContributionStep step,
    const int32_t retry_count,
    ledger::ResultCallback callback) {
  if (contribution_id.empty()) {
    BLOG(1, "Contribution id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "UPDATE %s SET step=?, retry_count=? WHERE contribution_id = ?;",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(step));
  BindInt(command.get(), 1, retry_count);
  BindString(command.get(), 2, contribution_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseContributionInfo::UpdateContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  publishers_->UpdateContributedAmount(
      contribution_id,
      publisher_key,
      callback);
}

void DatabaseContributionInfo::FinishAllInProgressRecords(
    ledger::ResultCallback callback) {
  auto transaction = ledger::DBTransaction::New();
  const std::string query = base::StringPrintf(
    "UPDATE %s SET step = ?, retry_count = 0 WHERE step >= 0",
    kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(
      command.get(),
      0,
      static_cast<int>(ledger::ContributionStep::STEP_REWARDS_OFF));

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

}  // namespace braveledger_database

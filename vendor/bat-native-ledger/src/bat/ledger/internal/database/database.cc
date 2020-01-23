/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/database/database_activity_info.h"
#include "bat/ledger/internal/database/database_initialize.h"
#include "bat/ledger/internal/database/database_publisher_info.h"
#include "bat/ledger/internal/database/database_recurring_tip.h"
#include "bat/ledger/internal/database/database_server_publisher_info.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_database {

Database::Database(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);

  initialize_ = std::make_unique<DatabaseInitialize>(ledger_);
  activity_info_ = std::make_unique<DatabaseActivityInfo>(ledger_);
  publisher_info_ = std::make_unique<DatabasePublisherInfo>(ledger_);
  recurring_tip_ = std::make_unique<DatabaseRecurringTip>(ledger_);
  server_publisher_info_ =
      std::make_unique<DatabaseServerPublisherInfo>(ledger_);
}

Database::~Database() = default;

void Database::Initialize(
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  initialize_->Start(execute_create_script, callback);
}

/**
 * ACTIVITY INFO
 */
void Database::SaveActivityInfo(
    ledger::PublisherInfoPtr info,
    ledger::ResultCallback callback) {
  ledger::PublisherInfoList list;
  list.push_back(std::move(info));
  activity_info_->InsertOrUpdateList(std::move(list), callback);
}

void Database::SaveActivityInfoList(
    ledger::PublisherInfoList list,
    ledger::ResultCallback callback) {
  activity_info_->InsertOrUpdateList(std::move(list), callback);
}

void Database::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  activity_info_->GetRecordsList(start, limit, std::move(filter), callback);
}

void Database::DeleteActivityInfo(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  activity_info_->DeleteRecord(publisher_key, callback);
}

/**
 * PUBLISHER INFO
 */
void Database::SavePublisherInfo(
    ledger::PublisherInfoPtr publisher_info,
    ledger::ResultCallback callback) {
  publisher_info_->InsertOrUpdate(std::move(publisher_info), callback);
}

void Database::GetPublisherInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  publisher_info_->GetRecord(publisher_key, callback);
}

void Database::GetPanelPublisherInfo(
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoCallback callback) {
  publisher_info_->GetPanelRecord(std::move(filter), callback);
}

void Database::RestorePublishers(ledger::ResultCallback callback) {
  publisher_info_->RestorePublishers(callback);
}

void Database::GetExcludedList(ledger::PublisherInfoListCallback callback) {
  publisher_info_->GetExcludedList(callback);
}

/**
 * RECURRING TIPS
 */
void Database::SaveRecurringTip(
    ledger::RecurringTipPtr info,
    ledger::ResultCallback callback) {
  recurring_tip_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetRecurringTips(ledger::PublisherInfoListCallback callback) {
  recurring_tip_->GetAllRecords(callback);
}

void Database::RemoveRecurringTip(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  recurring_tip_->DeleteRecord(publisher_key, callback);
}

/**
 * SERVER PUBLISHER INFO
 */
void Database::ClearAndInsertServerPublisherList(
    const ledger::ServerPublisherInfoList& list,
    ledger::ResultCallback callback) {
  server_publisher_info_->ClearAndInsertList(list, callback);
}

void Database::GetServerPublisherInfo(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) {
  server_publisher_info_->GetRecord(publisher_key, callback);
}

}  // namespace braveledger_database

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated_learning/brave_federated_data_service.h"

#include "base/task/thread_pool.h"
#include "base/threading/sequence_bound.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/brave_federated_learning/data_stores/ad_notification_timing_data_store.h"
#include "brave/components/brave_federated_learning/data_stores/data_store.h"
#include "sql/recovery.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave {

namespace federated_learning {

constexpr char kAdNotificationTaskName[] =
    "ad_notification_timing_federated_task";
constexpr char kAdNotificationTaskId[] = "0";
constexpr int kMaxNumberOfRecords = 50;
constexpr int kMaxRetentionDays = 30;

DataStoreService::DataStoreService(const base::FilePath& database_path)
    : db_path_(database_path),
      task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      ad_notification_timing_data_store_(task_runner_, db_path_) {}

DataStoreService::~DataStoreService() {
  EnforceRetentionPolicies();
}

void DataStoreService::OnInitComplete(bool success) {
  if (success)
    EnforceRetentionPolicies();
}

void DataStoreService::Init() {
  ad_notification_timing_data_store_
      .AsyncCall(&AdNotificationTimingDataStore::Init)
      .WithArgs(kAdNotificationTaskId, kAdNotificationTaskName,
                kMaxNumberOfRecords, kMaxRetentionDays)
      .Then(base::BindOnce(&DataStoreService::OnInitComplete,
                           base::Unretained(this)));
}

bool DataStoreService::DeleteDatabase() {
  return sql::Database::Delete(db_path_);
}

void DataStoreService::EnforceRetentionPolicies() {
  ad_notification_timing_data_store_.AsyncCall(
      &AdNotificationTimingDataStore::EnforceRetentionPolicy);
}

}  // namespace federated_learning

}  // namespace brave

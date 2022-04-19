/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/data_store_service.h"

#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequence_bound.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/brave_federated/data_stores/data_store.h"

namespace {
constexpr char kAdNotificationTaskName[] =
    "ad_notification_timing_task";
constexpr int kAdNotificationTaskId = 0;
constexpr int kMaxNumberOfRecords = 50;
constexpr int kMaxRetentionDays = 30;
}  // namespace

namespace brave_federated {

// -------------- AsyncDataStore ----------------------

AsyncDataStore::AsyncDataStore(base::FilePath db_path)
    : data_store_(base::ThreadPool::CreateSequencedTaskRunner(
                      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
                       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}),
                  db_path) {}

AsyncDataStore::~AsyncDataStore() {}

void AsyncDataStore::Init(int task_id,
                          const std::string& task_name,
                          int max_number_of_records,
                          int max_retention_days,
                          base::OnceCallback<void(bool)> callback) {
  data_store_.AsyncCall(&DataStore::Init)
      .WithArgs(task_id, task_name, max_number_of_records, max_retention_days)
      .Then(std::move(callback));
}

void AsyncDataStore::AddTrainingInstance(
    mojom::TrainingInstancePtr training_instance,
    base::OnceCallback<void(bool)> callback) {
  data_store_.AsyncCall(&DataStore::AddTrainingInstance)
      .WithArgs(std::move(training_instance))
      .Then(std::move(callback));
}

void AsyncDataStore::LoadTrainingData(
    base::OnceCallback<void(DataStore::TrainingData)> callback) {
  data_store_.AsyncCall(&DataStore::LoadTrainingData).Then(std::move(callback));
}

void AsyncDataStore::EnforceRetentionPolicy() {
  data_store_.AsyncCall(&DataStore::EnforceRetentionPolicy);
}

// -------------- DataStoreService --------------------

DataStoreService::DataStoreService(const base::FilePath& database_path)
    : db_path_(database_path),
      weak_factory_(this) {}

DataStoreService::~DataStoreService() {
  EnforceRetentionPolicies();
}

void DataStoreService::OnInitComplete(bool success) {
  if (success) {
    EnforceRetentionPolicies();
  }
}

void DataStoreService::Init() {
  data_stores_.clear();

  auto callback = base::BindOnce(&DataStoreService::OnInitComplete,
                                 weak_factory_.GetWeakPtr());
  AsyncDataStore ad_timing_data_store(db_path_);
  ad_timing_data_store.Init(kAdNotificationTaskId, kAdNotificationTaskName,
                             kMaxNumberOfRecords, kMaxRetentionDays,
                             std::move(callback));

  data_stores_.emplace(kAdNotificationTaskName, &ad_timing_data_store);
}

AsyncDataStore* DataStoreService::GetDataStore(const std::string& name) {
  if (data_stores_.find(name) == data_stores_.end())
    return nullptr;

  return data_stores_[name];
}

// AsyncDataStore* DataStoreService::GetDataStore(const std::string& name) {
//   return &ad_timing_data_store_;
// }

void DataStoreService::EnforceRetentionPolicies() {
  // ad_notification_timing_data_store_.EnforceRetentionPolicy();
}

}  // namespace brave_federated

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/data_store_service.h"

#include <memory>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequence_bound.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/brave_federated/data_stores/data_store.h"
#include "brave/components/brave_federated/tasks.h"

namespace {

}  // namespace

namespace brave_federated {

DataStoreService::DataStoreService(const base::FilePath& database_path)
    : db_path_(database_path), weak_factory_(this) {}

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
  std::unique_ptr<AsyncDataStore> ad_timing_data_store =
      std::make_unique<AsyncDataStore>(db_path_);
  ad_timing_data_store->Init(kAdNotificationTaskId, kAdNotificationTaskName,
                             kMaxNumberOfRecords, kMaxRetentionDays,
                             std::move(callback));

  data_stores_.emplace(kAdNotificationTaskName,
                       std::move(ad_timing_data_store));
}

AsyncDataStore* DataStoreService::GetDataStore(const std::string& name) {
  auto it = data_stores_.find(name);
  if (it == data_stores_.end())
    return nullptr;

  return it->second.get();
}

void DataStoreService::EnforceRetentionPolicies() {
  for (const auto& data_store : data_stores_) {
    data_store.second->EnforceRetentionPolicy();
  }
}

}  // namespace brave_federated

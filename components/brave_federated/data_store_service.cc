/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/data_store_service.h"

#include <utility>

#include "base/check.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_federated/data_stores/async_data_store.h"
#include "brave/components/brave_federated/notification_ad_task_constants.h"

namespace brave_federated {

DataStoreService::DataStoreService(const base::FilePath& db_path)
    : db_path_(db_path), weak_factory_(this) {}

DataStoreService::~DataStoreService() = default;

void DataStoreService::Init() {
  auto callback =
      base::BindOnce(&DataStoreService::OnInitializeDatabaseComplete,
                     weak_factory_.GetWeakPtr());
  DataStoreTask notification_ad_timing_data_store_task(
      {kNotificationAdTaskId, kNotificationAdTaskName, kMaxNumberOfRecords,
       kMaxRetentionDays});
  std::unique_ptr<AsyncDataStore> notification_ad_timing_data_store =
      std::make_unique<AsyncDataStore>(
          std::move(notification_ad_timing_data_store_task), db_path_);
  notification_ad_timing_data_store->InitializeDatabase(std::move(callback));

  data_stores_.emplace(kNotificationAdTaskName,
                       std::move(notification_ad_timing_data_store));
}

AsyncDataStore* DataStoreService::GetDataStore(const std::string& name) {
  auto it = data_stores_.find(name);
  if (it == data_stores_.end())
    return nullptr;

  return it->second.get();
}

void DataStoreService::OnInitializeDatabaseComplete(bool success) {
  if (success) {
    PurgeDataStoresAfterExpirationDate();
  }
}

void DataStoreService::PurgeDataStoresAfterExpirationDate() {
  for (const auto& entry : data_stores_) {
    AsyncDataStore* data_store = entry.second.get();
    DCHECK(data_store);
    data_store->PurgeTrainingDataAfterExpirationDate();
  }
}

}  // namespace brave_federated

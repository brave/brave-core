/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_

#include <string>
#include <utility>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequence_bound.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace brave_federated {

class AdNotificationTimingDataStore;
struct AdNotificationTimingTaskLog;

template <class T, class U>
class AsyncDataStore {
 private:
  base::SequenceBound<T> data_store_;

 public:
  explicit AsyncDataStore(base::FilePath db_path)
      : data_store_(base::ThreadPool::CreateSequencedTaskRunner(
                        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
                         base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}),
                    db_path) {}

  void Init(int task_id,
            const std::string& task_name,
            int max_number_of_records,
            int max_retention_days,
            base::OnceCallback<void(bool)> callback) {
    data_store_.AsyncCall(&T::Init)
        .WithArgs(task_id, task_name, max_number_of_records, max_retention_days)
        .Then(std::move(callback));
  }

  void AddLog(const U& log, base::OnceCallback<void(bool)> callback) {
    data_store_.AsyncCall(&T::AddLog).WithArgs(log).Then(std::move(callback));
  }

  void LoadLogs(base::OnceCallback<void(base::flat_map<int, U>)> callback) {
    data_store_.AsyncCall(&T::LoadLogs).Then(std::move(callback));
  }

  void EnforceRetentionPolicy() {
    data_store_.AsyncCall(&T::EnforceRetentionPolicy);
  }
};

// DataStoreService is the shared interface between all adopters applications
// (ads, news, etc.) and the task-specific data stores, which contains the task
// logs that are used to train and evaluate task-specific models.
class DataStoreService {
 public:
  explicit DataStoreService(const base::FilePath& base_database_path);
  ~DataStoreService();

  DataStoreService(const DataStoreService&) = delete;
  DataStoreService& operator=(const DataStoreService&) = delete;

  void Init();
  AsyncDataStore<AdNotificationTimingDataStore, AdNotificationTimingTaskLog>*
  GetAdNotificationTimingDataStore();

 private:
  void EnforceRetentionPolicies();
  void OnInitComplete(bool success);

  base::FilePath db_path_;
  AsyncDataStore<AdNotificationTimingDataStore, AdNotificationTimingTaskLog>
      ad_notification_timing_data_store_;
  base::WeakPtrFactory<DataStoreService> weak_factory_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_

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
#include "brave/components/brave_federated/data_stores/data_store.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"

namespace brave_federated {

class AsyncDataStore {
 public:
  explicit AsyncDataStore(base::FilePath db_path);
  ~AsyncDataStore();

  AsyncDataStore(const AsyncDataStore&) = delete;
  AsyncDataStore& operator=(const AsyncDataStore&) = delete;

  void Init(int task_id,
            const std::string& task_name,
            int max_number_of_records,
            int max_retention_days,
            base::OnceCallback<void(bool)> callback);

  void AddTrainingInstance(mojom::TrainingInstancePtr training_instance,
                           base::OnceCallback<void(bool)> callback);
  void LoadTrainingData(
      base::OnceCallback<void(DataStore::TrainingData)> callback);
  void EnforceRetentionPolicy();

 private:
  const base::SequenceBound<DataStore> data_store_;
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
  AsyncDataStore* GetDataStore(const std::string& name);

 private:
  void EnforceRetentionPolicies();
  void OnInitComplete(bool success);

  base::FilePath db_path_;
  base::flat_map<std::string, std::unique_ptr<AsyncDataStore>> data_stores_;
  base::WeakPtrFactory<DataStoreService> weak_factory_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_

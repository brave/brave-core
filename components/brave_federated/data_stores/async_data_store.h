/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_ASYNC_DATA_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_ASYNC_DATA_STORE_H_

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_federated/data_stores/data_store.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"

namespace brave_federated {

class DataStore;

// Wrapper around DataStore class to handle SequenceBound async logic
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

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_ASYNC_DATA_STORE_H_

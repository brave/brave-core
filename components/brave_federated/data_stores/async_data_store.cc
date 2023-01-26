/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/data_stores/async_data_store.h"

#include <utility>

#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"

namespace brave_federated {

AsyncDataStore::AsyncDataStore(DataStoreTask data_store_task,
                               base::FilePath db_path)
    : data_store_(base::ThreadPool::CreateSequencedTaskRunner(
                      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
                       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}),
                  data_store_task,
                  db_path) {}

AsyncDataStore::~AsyncDataStore() = default;

void AsyncDataStore::InitializeDatabase(
    base::OnceCallback<void(bool)> callback) {
  data_store_.AsyncCall(&DataStore::InitializeDatabase)
      .Then(std::move(callback));
}

void AsyncDataStore::AddTrainingInstance(
    std::vector<brave_federated::mojom::CovariateInfoPtr> training_instance,
    base::OnceCallback<void(bool)> callback) {
  data_store_.AsyncCall(&DataStore::AddTrainingInstance)
      .WithArgs(std::move(training_instance))
      .Then(std::move(callback));
}

void AsyncDataStore::LoadTrainingData(
    base::OnceCallback<void(TrainingData)> callback) {
  data_store_.AsyncCall(&DataStore::LoadTrainingData).Then(std::move(callback));
}

void AsyncDataStore::PurgeTrainingDataAfterExpirationDate() {
  data_store_.AsyncCall(&DataStore::PurgeTrainingDataAfterExpirationDate);
}

}  // namespace brave_federated

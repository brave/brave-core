/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/data_stores/async_data_store.h"

#include <utility>

#include "base/callback.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/sequence_bound.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace brave_federated {

AsyncDataStore::AsyncDataStore(base::FilePath db_path)
    : data_store_(base::ThreadPool::CreateSequencedTaskRunner(
                      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
                       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}),
                  db_path) {}

AsyncDataStore::~AsyncDataStore() = default;

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

void AsyncDataStore::PurgeTrainingDataAfterExpirationDate() {
  data_store_.AsyncCall(&DataStore::PurgeTrainingDataAfterExpirationDate);
}

}  // namespace brave_federated

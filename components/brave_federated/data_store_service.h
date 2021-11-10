/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/sequence_bound.h"

namespace brave_federated {

class AdNotificationTimingDataStore;

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
  base::SequenceBound<AdNotificationTimingDataStore>*
  GetAdNotificationTimingDataStore();

 private:
  void EnforceRetentionPolicies();
  void OnInitComplete(bool success);

  base::FilePath db_path_;
  base::SequenceBound<AdNotificationTimingDataStore>
      ad_notification_timing_data_store_;
  base::WeakPtrFactory<DataStoreService> weak_factory_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_

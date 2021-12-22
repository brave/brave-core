/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_DATA_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_DATA_SERVICE_H_

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/sequence_bound.h"
#include "sql/database.h"

namespace brave {

namespace federated_learning {

class AdNotificationTimingDataStore;

class DataStoreService {
 public:
  explicit DataStoreService(const base::FilePath& base_database_path);
  ~DataStoreService();

  DataStoreService(const DataStoreService&) = delete;
  DataStoreService& operator=(const DataStoreService&) = delete;

  void Init();

  bool DeleteDatabase();

 private:
  void EnforceRetentionPolicies();
  void OnInitComplete(bool success);

  base::FilePath db_path_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::SequenceBound<AdNotificationTimingDataStore>
      ad_notification_timing_data_store_;
};

}  // namespace federated_learning

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_DATA_SERVICE_H_

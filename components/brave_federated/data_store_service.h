/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"

namespace brave_federated {

class AsyncDataStore;

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
  void OnInitializeDatabaseComplete(bool success);
  void PurgeDataStoresAfterExpirationDate();

  base::FilePath db_path_;
  base::flat_map<std::string, std::unique_ptr<AsyncDataStore>> data_stores_;
  base::WeakPtrFactory<DataStoreService> weak_factory_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORE_SERVICE_H_

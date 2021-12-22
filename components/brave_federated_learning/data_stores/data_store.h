/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_DATA_STORES_DATA_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_DATA_STORES_DATA_STORE_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "sql/database.h"

namespace brave {

namespace federated_learning {

class DataStore {
 public:
  explicit DataStore(const base::FilePath& database_path);

  DataStore(const DataStore&) = delete;
  DataStore& operator=(const DataStore&) = delete;

  bool Init(const std::string& task_id,
            const std::string& task_name,
            const int max_number_of_records,
            const int max_retention_days);
  bool DeleteLogs();
  void EnforceRetentionPolicy();

  virtual ~DataStore();
  virtual bool EnsureTable();

  sql::Database db_;
  base::FilePath database_path_;

  std::string task_id_;
  std::string task_name_;
  int max_number_of_records_;
  int max_retention_days_;
};

}  // namespace federated_learning

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_DATA_STORES_DATA_STORE_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_DATA_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_DATA_STORE_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/sequence_checker.h"
#include "sql/database.h"

namespace brave_federated {

class DataStore {
 public:
  explicit DataStore(const base::FilePath& database_path);

  DataStore(const DataStore&) = delete;
  DataStore& operator=(const DataStore&) = delete;

  bool Init(int task_id,
            const std::string& task_name,
            int max_number_of_records,
            int max_retention_days);
  bool DeleteLogs();
  void EnforceRetentionPolicy();

  virtual ~DataStore();

 protected:
  sql::Database db_;
  base::FilePath database_path_;

  std::string task_id_;
  std::string task_name_;
  int max_number_of_records_;
  int max_retention_days_;

 private:
  virtual bool EnsureTable();

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_DATA_STORE_H_

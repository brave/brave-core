/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_DATA_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_DATA_STORE_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"
#include "sql/database.h"

namespace brave_federated {

using TrainingData = base::flat_map<int, std::vector<mojom::CovariateInfoPtr>>;

struct DataStoreTask {
  int id = 0;
  const std::string name;
  int max_number_of_records = 0;
  base::TimeDelta max_retention_days;
};

class DataStore {
 public:
  explicit DataStore(const DataStoreTask data_store_task,
                     const base::FilePath& db_file_path);
  ~DataStore();

  DataStore(const DataStore&) = delete;
  DataStore& operator=(const DataStore&) = delete;

  bool InitializeDatabase();

  int GetNextTrainingInstanceId();
  void SaveCovariate(const brave_federated::mojom::CovariateInfo& covariate,
                     int training_instance_id,
                     const base::Time created_at);
  bool AddTrainingInstance(
      const std::vector<brave_federated::mojom::CovariateInfoPtr>
          training_instance);

  bool DeleteTrainingData();
  TrainingData LoadTrainingData();
  void PurgeTrainingDataAfterExpirationDate();

 protected:
  friend class DataStoreTest;

  sql::Database database_;
  base::FilePath db_file_path_;
  DataStoreTask data_store_task_;

 private:
  bool MaybeCreateTable();

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_DATA_STORES_DATA_STORE_H_

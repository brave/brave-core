/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "brave/components/brave_federated/data_stores/data_store.h"
#include "content/public/test/browser_task_environment.h"
#include "sql/statement.h"
#include "sql/test/scoped_error_expecter.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=DataStoreTest*

namespace brave_federated {

namespace {

struct TrainingDataInfo {
  int training_instance_id;
  int feature_name;
  int feature_type;
  std::string feature_value;
} kTrainingData[] = {
    {0, 1, 0, "cat"},
    {0, 2, 1, "24"},
    {1, 1, 0, "dog"},
    {1, 2, 1, "42"},
};

}  // namespace

class DataStoreTest : public testing::Test {
 public:
  DataStoreTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  void SetUp() override;

  int RecordCount() const;
  int TrainingInstanceCount() const;

  TrainingData TrainingDataFromTestInfo();

  bool AddTrainingInstance(std::vector<mojom::CovariateInfoPtr> covariates);
  void InitializeDataStore();

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<DataStore> data_store_;
};

void DataStoreTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("test_data_store")));
  DataStoreTask data_store_task({0, "test_federated_task",
                                 /* max_number_of_records */ 50,
                                 base::Days(30)});
  data_store_ =
      std::make_unique<DataStore>(std::move(data_store_task), db_path);
  ASSERT_TRUE(data_store_->InitializeDatabase());
}

int DataStoreTest::RecordCount() const {
  sql::Statement statement(data_store_->database_.GetUniqueStatement(
      "SELECT count(*) FROM test_federated_task"));
  const bool success = statement.Step();
  DCHECK(success);
  return statement.ColumnInt(0);
}

int DataStoreTest::TrainingInstanceCount() const {
  sql::Statement statement(data_store_->database_.GetUniqueStatement(
      "SELECT count(DISTINCT training_instance_id) FROM test_federated_task"));
  const bool success = statement.Step();
  DCHECK(success);
  return statement.ColumnInt(0);
}

TrainingData DataStoreTest::TrainingDataFromTestInfo() {
  TrainingData training_data;

  for (const auto& item : kTrainingData) {
    int training_instance_id = item.training_instance_id;
    mojom::CovariateInfoPtr covariate = mojom::CovariateInfo::New();
    covariate->type = (mojom::CovariateType)item.feature_name;
    covariate->data_type = (mojom::DataType)item.feature_type;
    covariate->value = item.feature_value;

    training_data[training_instance_id].push_back(std::move(covariate));
  }

  return training_data;
}

void DataStoreTest::InitializeDataStore() {
  TrainingData training_data = TrainingDataFromTestInfo();
  for (auto& training_instance_pair : training_data) {
    auto& training_instance = training_instance_pair.second;
    AddTrainingInstance(std::move(training_instance));
  }
  EXPECT_EQ(sizeof(kTrainingData) / sizeof(kTrainingData[0]),
            static_cast<unsigned int>(RecordCount()));
  EXPECT_EQ(training_data.size(),
            static_cast<unsigned int>(TrainingInstanceCount()));
}

bool DataStoreTest::AddTrainingInstance(
    std::vector<mojom::CovariateInfoPtr> covariates) {
  std::vector<brave_federated::mojom::CovariateInfoPtr> training_instance;
  for (auto& covariate : covariates) {
    training_instance.push_back(std::move(covariate));
  }

  return data_store_->AddTrainingInstance(std::move(training_instance));
}

// Actual tests
// -------------------------------------------------------------------------------------

TEST_F(DataStoreTest, GetNextTrainingInstanceIdWhenDatabaseEmpty) {
  EXPECT_EQ(1, data_store_->GetNextTrainingInstanceId());
}

TEST_F(DataStoreTest, GetNextTrainingInstanceId) {
  TrainingData training_data = TrainingDataFromTestInfo();
  EXPECT_TRUE(AddTrainingInstance(std::move(training_data[0])));
  EXPECT_EQ(1, TrainingInstanceCount());
  EXPECT_EQ(2, data_store_->GetNextTrainingInstanceId());
}

TEST_F(DataStoreTest, SaveCovariate) {
  TrainingData training_data = TrainingDataFromTestInfo();
  data_store_->SaveCovariate(*training_data[0][0], 0, base::Time::Now());
  EXPECT_EQ(1, TrainingInstanceCount());
  data_store_->SaveCovariate(*training_data[0][0], 0, base::Time::Now());
  EXPECT_EQ(1, TrainingInstanceCount());
  data_store_->SaveCovariate(*training_data[0][0], 1, base::Time::Now());
  EXPECT_EQ(2, TrainingInstanceCount());
}

TEST_F(DataStoreTest, AddTrainingInstance) {
  TrainingData training_data = TrainingDataFromTestInfo();
  EXPECT_TRUE(AddTrainingInstance(std::move(training_data[0])));
  EXPECT_EQ(1, TrainingInstanceCount());
  EXPECT_TRUE(AddTrainingInstance(std::move(training_data[1])));
  EXPECT_EQ(2, TrainingInstanceCount());
}

TEST_F(DataStoreTest, LoadTrainingData) {
  InitializeDataStore();
  EXPECT_EQ(4, RecordCount());
  EXPECT_EQ(2, TrainingInstanceCount());
  auto training_data = data_store_->LoadTrainingData();

  for (unsigned int i = 0;
       i < sizeof(kTrainingData) / sizeof(kTrainingData[0]) / 2; ++i) {
    auto it = training_data.find(i + 1);
    EXPECT_EQ(2U, it->second.size());
    EXPECT_TRUE(it != training_data.end());
  }
}

TEST_F(DataStoreTest, DeleteLogs) {
  InitializeDataStore();
  EXPECT_EQ(4, RecordCount());
  TrainingData training_data = data_store_->LoadTrainingData();
  EXPECT_EQ(training_data.size(),
            static_cast<unsigned int>(TrainingInstanceCount()));
  EXPECT_TRUE(data_store_->DeleteTrainingData());
  EXPECT_EQ(0, RecordCount());
  training_data = data_store_->LoadTrainingData();
  EXPECT_EQ(0, RecordCount());
}

TEST_F(DataStoreTest, PurgeTrainingDataAfterExpirationDate) {
  InitializeDataStore();
  EXPECT_EQ(4, RecordCount());
  task_environment_.AdvanceClock(base::Days(31));

  data_store_->PurgeTrainingDataAfterExpirationDate();

  TrainingData training_data = data_store_->LoadTrainingData();
  EXPECT_EQ(0, RecordCount());
}

}  // namespace brave_federated

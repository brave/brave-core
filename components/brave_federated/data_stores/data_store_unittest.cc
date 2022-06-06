/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

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

namespace {

struct TrainingDataTestInfo {
  int training_instance_id;
  int feature_name;
  int feature_type;
  std::string feature_value;
} kTestTrainingData[] = {
    {0, 1, 0, "cat"},
    {0, 2, 1, "24"},
    {1, 1, 0, "dog"},
    {1, 2, 1, "42"},
};

}  // namespace

namespace brave_federated {

class DataStoreTest : public testing::Test {
 public:
  DataStoreTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  void SetUp() override;
  void TearDown() override;

  void ClearDB();
  size_t CountRecords() const;
  size_t CountTrainingInstances() const;

  DataStore::TrainingData TrainingDataFromTestInfo();

  bool AddTrainingInstance(std::vector<mojom::CovariatePtr> covariates);
  void InitializeDataStore();

  content::BrowserTaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  DataStore* data_store_;
};

void DataStoreTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("test_data_store")));
  data_store_ = new DataStore(db_path);
  ASSERT_TRUE(data_store_->Init(0, "test_federated_task", 50, 30));
  ClearDB();
}

void DataStoreTest::TearDown() {
  data_store_ = nullptr;
}

void DataStoreTest::ClearDB() {
  EXPECT_TRUE(data_store_->DeleteTrainingData());
}

size_t DataStoreTest::CountRecords() const {
  sql::Statement s(data_store_->db_.GetUniqueStatement(
      "SELECT count(*) FROM test_federated_task"));
  EXPECT_TRUE(s.Step());
  return static_cast<size_t>(s.ColumnInt(0));
}

size_t DataStoreTest::CountTrainingInstances() const {
  sql::Statement s(data_store_->db_.GetUniqueStatement(
      "SELECT count(DISTINCT training_instance_id) FROM test_federated_task"));
  EXPECT_TRUE(s.Step());
  return static_cast<size_t>(s.ColumnInt(0));
}

DataStore::TrainingData DataStoreTest::TrainingDataFromTestInfo() {
  DataStore::TrainingData training_data;

  for (const auto& test_entry : kTestTrainingData) {
    int training_instance_id = test_entry.training_instance_id;
    mojom::CovariatePtr covariate = mojom::Covariate::New();
    covariate->covariate_type = (mojom::CovariateType)test_entry.feature_name;
    covariate->data_type = (mojom::DataType)test_entry.feature_type;
    covariate->value = test_entry.feature_value;

    training_data[training_instance_id].push_back(std::move(covariate));
  }

  return training_data;
}

bool DataStoreTest::AddTrainingInstance(
    std::vector<mojom::CovariatePtr> covariates) {
  mojom::TrainingInstancePtr training_instance = mojom::TrainingInstance::New();
  for (const auto& covariate_data : covariates) {
    mojom::CovariatePtr covariate = mojom::Covariate::New();
    covariate->covariate_type = covariate_data->covariate_type;
    covariate->data_type = covariate_data->data_type;
    covariate->value = covariate_data->value;
    training_instance->covariates.push_back(std::move(covariate));
  }

  return data_store_->AddTrainingInstance(std::move(training_instance));
}

void DataStoreTest::InitializeDataStore() {
  ClearDB();
  DataStore::TrainingData training_data = TrainingDataFromTestInfo();
  for (const auto& training_instance : training_data) {
    AddTrainingInstance(std::move(training_instance.second));
  }
  EXPECT_EQ(std::size(kTestTrainingData), CountRecords());
  EXPECT_EQ(std::size(training_data), CountTrainingInstances());
}

// Actual tests
// -------------------------------------------------------------------------------------

TEST_F(DataStoreTest, AddTrainingInstance) {
  ClearDB();
  EXPECT_EQ(0U, CountRecords());
  DataStore::TrainingData training_data = TrainingDataFromTestInfo();
  EXPECT_TRUE(AddTrainingInstance(std::move(training_data[0])));
  EXPECT_EQ(1U, CountTrainingInstances());
  EXPECT_TRUE(AddTrainingInstance(std::move(training_data[1])));
  EXPECT_EQ(2U, CountTrainingInstances());
}

TEST_F(DataStoreTest, LoadTrainingData) {
  InitializeDataStore();
  EXPECT_EQ(4U, CountRecords());
  EXPECT_EQ(2U, CountTrainingInstances());
  auto training_data = data_store_->LoadTrainingData();

  for (size_t i = 0; i < std::size(kTestTrainingData) / 2; ++i) {
    auto it = training_data.find(i + 1);
    EXPECT_EQ(2U, std::size(it->second));
    EXPECT_TRUE(it != training_data.end());
  }
}

TEST_F(DataStoreTest, DeleteLogs) {
  InitializeDataStore();
  EXPECT_EQ(4U, CountRecords());
  DataStore::TrainingData training_data = data_store_->LoadTrainingData();
  EXPECT_EQ(training_data.size(), CountTrainingInstances());
  EXPECT_TRUE(data_store_->DeleteTrainingData());
  EXPECT_EQ(0U, CountRecords());
  training_data = data_store_->LoadTrainingData();
  EXPECT_EQ(0U, CountRecords());
}

TEST_F(DataStoreTest, EnforceRetentionPolicy) {
  InitializeDataStore();
  EXPECT_EQ(4U, CountRecords());
  task_environment_.AdvanceClock(base::Days(31));

  data_store_->EnforceRetentionPolicy();

  DataStore::TrainingData training_data = data_store_->LoadTrainingData();
  EXPECT_EQ(0U, CountRecords());
}

}  // namespace brave_federated

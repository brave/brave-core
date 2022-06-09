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

  void ClearDatabase();
  size_t RecordCount() const;
  size_t TrainingInstanceCount() const;

  DataStore::TrainingData TrainingDataFromTestInfo();

  bool AddTrainingInstance(std::vector<mojom::CovariatePtr> covariates);
  void InitializeDataStore();

  content::BrowserTaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  DataStore* data_store_ = nullptr;
};

void DataStoreTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("test_data_store")));
  data_store_ = new DataStore(db_path);
  ASSERT_TRUE(data_store_->Init(0, "test_federated_task", 50, 30));
  ClearDatabase();
}

void DataStoreTest::ClearDatabase() {
  EXPECT_TRUE(data_store_->DeleteTrainingData());
}

size_t DataStoreTest::RecordCount() const {
  sql::Statement statement(data_store_->db_.GetUniqueStatement(
      "SELECT count(*) FROM test_federated_task"));
  EXPECT_TRUE(statement.Step());
  return static_cast<size_t>(statement.ColumnInt(0));
}

size_t DataStoreTest::TrainingInstanceCount() const {
  sql::Statement statement(data_store_->db_.GetUniqueStatement(
      "SELECT count(DISTINCT training_instance_id) FROM test_federated_task"));
  EXPECT_TRUE(statement.Step());
  return static_cast<size_t>(statement.ColumnInt(0));
}

DataStore::TrainingData DataStoreTest::TrainingDataFromTestInfo() {
  DataStore::TrainingData training_data;

  for (const auto& test_entry : kTestTrainingData) {
    int training_instance_id = test_entry.training_instance_id;
    mojom::CovariatePtr covariate = mojom::Covariate::New();
    covariate->type = (mojom::CovariateType)test_entry.feature_name;
    covariate->data_type = (mojom::DataType)test_entry.feature_type;
    covariate->value = test_entry.feature_value;

    training_data[training_instance_id].push_back(std::move(covariate));
  }

  return training_data;
}

void DataStoreTest::InitializeDataStore() {
  ClearDatabase();
  DataStore::TrainingData training_data = TrainingDataFromTestInfo();
  for (auto& training_instance_pair : training_data) {
    auto& training_instance = training_instance_pair.second;
    AddTrainingInstance(std::move(training_instance));
  }
  EXPECT_EQ(std::size(kTestTrainingData), RecordCount());
  EXPECT_EQ(std::size(training_data), TrainingInstanceCount());
}

bool DataStoreTest::AddTrainingInstance(
    std::vector<mojom::CovariatePtr> covariates) {
  mojom::TrainingInstancePtr training_instance = mojom::TrainingInstance::New();
  for (auto& covariate : covariates) {
    training_instance->covariates.push_back(std::move(covariate));
  }

  return data_store_->AddTrainingInstance(std::move(training_instance));
}

// Actual tests
// -------------------------------------------------------------------------------------

TEST_F(DataStoreTest, AddTrainingInstance) {
  ClearDatabase();
  EXPECT_EQ(0U, RecordCount());
  DataStore::TrainingData training_data = TrainingDataFromTestInfo();
  EXPECT_TRUE(AddTrainingInstance(std::move(training_data[0])));
  EXPECT_EQ(1U, TrainingInstanceCount());
  EXPECT_TRUE(AddTrainingInstance(std::move(training_data[1])));
  EXPECT_EQ(2U, TrainingInstanceCount());
}

TEST_F(DataStoreTest, LoadTrainingData) {
  InitializeDataStore();
  EXPECT_EQ(4U, RecordCount());
  EXPECT_EQ(2U, TrainingInstanceCount());
  auto training_data = data_store_->LoadTrainingData();

  for (size_t i = 0; i < std::size(kTestTrainingData) / 2; ++i) {
    auto it = training_data.find(i + 1);
    EXPECT_EQ(2U, std::size(it->second));
    EXPECT_TRUE(it != training_data.end());
  }
}

TEST_F(DataStoreTest, DeleteLogs) {
  InitializeDataStore();
  EXPECT_EQ(4U, RecordCount());
  DataStore::TrainingData training_data = data_store_->LoadTrainingData();
  EXPECT_EQ(training_data.size(), TrainingInstanceCount());
  EXPECT_TRUE(data_store_->DeleteTrainingData());
  EXPECT_EQ(0U, RecordCount());
  training_data = data_store_->LoadTrainingData();
  EXPECT_EQ(0U, RecordCount());
}

TEST_F(DataStoreTest, PurgeTrainingDataAfterExpirationDate) {
  InitializeDataStore();
  EXPECT_EQ(4U, RecordCount());
  task_environment_.AdvanceClock(base::Days(31));

  data_store_->PurgeTrainingDataAfterExpirationDate();

  DataStore::TrainingData training_data = data_store_->LoadTrainingData();
  EXPECT_EQ(0U, RecordCount());
}

}  // namespace brave_federated

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "brave/components/brave_federated/data_stores/test_data_store.h"
#include "sql/statement.h"
#include "sql/test/scoped_error_expecter.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=DataStoreTest*

namespace {

struct TestTaskLogTestInfo {
  int id;
  int days_from_now;
  bool label;
} test_task_log_test_db[] = {
    {0, 31, true},
    {0, 15, false},
    {0, 7, false},
    {0, 1, true},
};

}  // namespace

namespace brave_federated {

class DataStoreTest : public testing::Test {
 public:
  void SetUp() override;
  void TearDown() override;

  void ClearDB();
  size_t CountRecords() const;

  TestTaskLog TestTaskLogFromTestInfo(const TestTaskLogTestInfo& info);

  void AddAll();

  base::ScopedTempDir temp_dir_;
  TestDataStore* test_data_store_;
};

void DataStoreTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("test_data_store")));
  test_data_store_ = new TestDataStore(db_path);
  ASSERT_TRUE(test_data_store_->Init(0, "test_federated_task", 50, 30));
  ClearDB();
}

void DataStoreTest::TearDown() {
  test_data_store_ = nullptr;
}

void DataStoreTest::ClearDB() {
  EXPECT_TRUE(test_data_store_->DeleteLogs());
}

size_t DataStoreTest::CountRecords() const {
  sql::Statement s(test_data_store_->db_.GetUniqueStatement(
      "SELECT count(*) FROM test_federated_task"));
  EXPECT_TRUE(s.Step());
  return static_cast<size_t>(s.ColumnInt(0));
}

TestTaskLog DataStoreTest::TestTaskLogFromTestInfo(
    const TestTaskLogTestInfo& info) {
  return TestTaskLog(info.id, info.label,
                     base::Time::Now() - base::Days(info.days_from_now));
}

void DataStoreTest::AddAll() {
  ClearDB();
  for (size_t i = 0; i < std::size(test_task_log_test_db); ++i) {
    TestTaskLog test_log = TestTaskLogFromTestInfo(test_task_log_test_db[i]);
    test_data_store_->AddLog(test_log);
  }
  EXPECT_EQ(std::size(test_task_log_test_db), CountRecords());
}

// Actual tests
// -------------------------------------------------------------------------------------

TEST_F(DataStoreTest, DeleteLogs) {
  AddAll();
  EXPECT_EQ(4U, CountRecords());
  TestDataStore::IdToTestTaskLogMap test_logs;
  test_data_store_->LoadLogs(&test_logs);
  EXPECT_EQ(test_logs.size(), CountRecords());
  EXPECT_TRUE(test_data_store_->DeleteLogs());
  EXPECT_EQ(0U, CountRecords());
  test_data_store_->LoadLogs(&test_logs);
  EXPECT_EQ(0U, CountRecords());
}

TEST_F(DataStoreTest, EnforceRetentionPolicy) {
  AddAll();
  EXPECT_EQ(4U, CountRecords());

  test_data_store_->EnforceRetentionPolicy();

  TestDataStore::IdToTestTaskLogMap test_logs;
  test_data_store_->LoadLogs(&test_logs);
  EXPECT_EQ(3U, CountRecords());

  auto it = test_logs.find(1);
  EXPECT_TRUE(it == test_logs.end());
}

}  // namespace brave_federated

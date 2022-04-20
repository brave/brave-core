/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/data_stores/ad_notification_timing_data_store.h"

#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "sql/statement.h"
#include "sql/test/scoped_error_expecter.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=AdNotificationTimingDataStoreTest*

namespace {

struct AdNotificationTimingTaskLogTestInfo {
  int id;
  int days_from_now;
  std::string locale;
  bool label;
  int number_of_tabs;
} ad_notification_task_log_test_db[] = {
    {0, 31, "US", true, 23},
    {0, 15, "US", false, 10},
    {0, 7, "GB", false, 4},
    {0, 1, "IT", true, 72},
};

}  // namespace

namespace brave_federated {

class AdNotificationTimingDataStoreTest : public testing::Test {
 public:
  void SetUp() override;
  void TearDown() override;

  void ClearDB();
  size_t CountRecords() const;

  AdNotificationTimingTaskLog AdNotificationTimingTaskLogFromTestInfo(
      const AdNotificationTimingTaskLogTestInfo& info);

  void AddAll();

  base::ScopedTempDir temp_dir_;
  AdNotificationTimingDataStore* ad_notification_data_store_;
};

void AdNotificationTimingDataStoreTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("ad_notification_test")));
  ad_notification_data_store_ = new AdNotificationTimingDataStore(db_path);
  ASSERT_TRUE(ad_notification_data_store_->Init(
      0, "ad_notification_timing_federated_task", 50, 30));
  ClearDB();
}

void AdNotificationTimingDataStoreTest::TearDown() {
  ad_notification_data_store_ = nullptr;
}

void AdNotificationTimingDataStoreTest::ClearDB() {
  EXPECT_TRUE(ad_notification_data_store_->DeleteLogs());
}

size_t AdNotificationTimingDataStoreTest::CountRecords() const {
  sql::Statement s(ad_notification_data_store_->db_.GetUniqueStatement(
      "SELECT count(*) FROM ad_notification_timing_federated_task"));
  EXPECT_TRUE(s.Step());
  return static_cast<size_t>(s.ColumnInt(0));
}

AdNotificationTimingTaskLog
AdNotificationTimingDataStoreTest::AdNotificationTimingTaskLogFromTestInfo(
    const AdNotificationTimingTaskLogTestInfo& info) {
  return AdNotificationTimingTaskLog(
      info.id, base::Time::FromInternalValue(info.days_from_now), info.locale,
      info.number_of_tabs, info.label,
      base::Time::Now() - base::Days(info.days_from_now));
}

void AdNotificationTimingDataStoreTest::AddAll() {
  ClearDB();
  for (size_t i = 0; i < std::size(ad_notification_task_log_test_db); ++i)
    ad_notification_data_store_->AddLog(AdNotificationTimingTaskLogFromTestInfo(
        ad_notification_task_log_test_db[i]));
  EXPECT_EQ(std::size(ad_notification_task_log_test_db), CountRecords());
}

// Actual tests
// -------------------------------------------------------------------------------------

TEST_F(AdNotificationTimingDataStoreTest, CheckSchemaColumnExistence) {
  ASSERT_TRUE(ad_notification_data_store_->db_.DoesColumnExist(
      "ad_notification_timing_federated_task", "id"));
  ASSERT_TRUE(ad_notification_data_store_->db_.DoesColumnExist(
      "ad_notification_timing_federated_task", "time"));
  ASSERT_TRUE(ad_notification_data_store_->db_.DoesColumnExist(
      "ad_notification_timing_federated_task", "locale"));
  ASSERT_TRUE(ad_notification_data_store_->db_.DoesColumnExist(
      "ad_notification_timing_federated_task", "number_of_tabs"));
  ASSERT_TRUE(ad_notification_data_store_->db_.DoesColumnExist(
      "ad_notification_timing_federated_task", "label"));
  ASSERT_TRUE(ad_notification_data_store_->db_.DoesColumnExist(
      "ad_notification_timing_federated_task", "creation_date"));
}

TEST_F(AdNotificationTimingDataStoreTest, AddLog) {
  ClearDB();
  EXPECT_EQ(0U, CountRecords());
  EXPECT_TRUE(ad_notification_data_store_->AddLog(
      AdNotificationTimingTaskLogFromTestInfo(
          ad_notification_task_log_test_db[0])));
  EXPECT_EQ(1U, CountRecords());
  EXPECT_TRUE(ad_notification_data_store_->AddLog(
      AdNotificationTimingTaskLogFromTestInfo(
          ad_notification_task_log_test_db[1])));
  EXPECT_EQ(2U, CountRecords());
}

TEST_F(AdNotificationTimingDataStoreTest, LoadLogs) {
  AddAll();
  EXPECT_EQ(4U, CountRecords());
  auto ad_notification_timing_logs = ad_notification_data_store_->LoadLogs();

  for (size_t i = 0; i < std::size(ad_notification_task_log_test_db); ++i) {
    auto it = ad_notification_timing_logs.find(i + 1);
    EXPECT_TRUE(it != ad_notification_timing_logs.end());
  }
}

}  // namespace brave_federated

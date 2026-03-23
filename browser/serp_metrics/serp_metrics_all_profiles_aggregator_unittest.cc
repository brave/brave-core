/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_all_profiles_aggregator.h"

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "brave/browser/serp_metrics/serp_metrics_time_period_store_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

namespace {
constexpr base::FilePath::CharType kUserDataDir[] =
    FILE_PATH_LITERAL("/testing_user_data_dir");
}  // namespace

class SerpMetricsAllProfilesAggregatorTest : public ::testing::Test {
 public:
  SerpMetricsAllProfilesAggregatorTest() = default;
  ~SerpMetricsAllProfilesAggregatorTest() override = default;

  void SetUp() override {
    local_state_.registry()->RegisterStringPref(kLastCheckYMD,
                                                "");  // Never checked.
    ProfileAttributesStorage::RegisterPrefs(local_state_.registry());
    storage_ = std::make_unique<ProfileAttributesStorage>(
        &local_state_, base::FilePath(kUserDataDir));
  }

  void AddProfile(const base::FilePath& profile_path) {
    ProfileAttributesInitParams profile_init_params;
    profile_init_params.profile_path = profile_path;
    storage_->AddProfile(std::move(profile_init_params));
  }

  // Advances the clock to the start of a brand new day.
  void AdvanceClockToNextDay() {
    task_environment_.AdvanceClock(base::Days(1));
  }

  PrefService* local_state() { return &local_state_; }

  ProfileAttributesStorage& profile_attributes_storage() { return *storage_; }

 private:
  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<ProfileAttributesStorage> storage_;
};

TEST_F(SerpMetricsAllProfilesAggregatorTest,
       AggregateNoMetricsForSingleProfile) {
  base::FilePath profile_path =
      base::FilePath(kUserDataDir).AppendASCII("testing_profile");
  AddProfile(profile_path);

  SerpMetricsAllProfilesAggregator aggregator(local_state(),
                                              profile_attributes_storage());
  EXPECT_EQ(0U, aggregator.GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, aggregator.GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(0U, aggregator.GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(0U, aggregator.GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsAllProfilesAggregatorTest,
       AggregateYesterdayRecordedMetricsForSingleProfile) {
  base::FilePath profile_path =
      base::FilePath(kUserDataDir).AppendASCII("testing_profile");
  AddProfile(profile_path);
  std::unique_ptr<SerpMetrics> serp_metrics = std::make_unique<SerpMetrics>(
      local_state(), SerpMetricsTimePeriodStoreFactory(
                         profile_path, profile_attributes_storage()));

  // Day 0: Stale
  serp_metrics->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics->RecordSearch(SerpMetricType::kBrave);
  serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics->RecordSearch(SerpMetricType::kGoogle);

  SerpMetricsAllProfilesAggregator aggregator(local_state(),
                                              profile_attributes_storage());
  EXPECT_EQ(1U, aggregator.GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U, aggregator.GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U, aggregator.GetSearchCountForYesterday(SerpMetricType::kOther));
}

TEST_F(SerpMetricsAllProfilesAggregatorTest,
       AggregateStalePeriodRecordedMetricsForSingleProfile) {
  base::FilePath profile_path =
      base::FilePath(kUserDataDir).AppendASCII("testing_profile");
  AddProfile(profile_path);
  std::unique_ptr<SerpMetrics> serp_metrics = std::make_unique<SerpMetrics>(
      local_state(), SerpMetricsTimePeriodStoreFactory(
                         profile_path, profile_attributes_storage()));

  // Day 0: Stale
  serp_metrics->RecordSearch(SerpMetricType::kBrave);
  serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics->RecordSearch(SerpMetricType::kBrave);
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics->RecordSearch(SerpMetricType::kGoogle);

  SerpMetricsAllProfilesAggregator aggregator(local_state(),
                                              profile_attributes_storage());
  EXPECT_EQ(3U, aggregator.GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsAllProfilesAggregatorTest,
       AggregateNoMetricsForMultipleProfiles) {
  base::FilePath profile_path_1 =
      base::FilePath(kUserDataDir).AppendASCII("testing_profile_1");
  AddProfile(profile_path_1);
  base::FilePath profile_path_2 =
      base::FilePath(kUserDataDir).AppendASCII("testing_profile_2");
  AddProfile(profile_path_2);

  SerpMetricsAllProfilesAggregator aggregator(local_state(),
                                              profile_attributes_storage());

  EXPECT_EQ(0U, aggregator.GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U, aggregator.GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsAllProfilesAggregatorTest,
       AggregateYesterdayRecordedMetricsForMultipleProfiles) {
  base::FilePath profile_path_1 =
      base::FilePath(kUserDataDir).AppendASCII("testing_profile_1");
  AddProfile(profile_path_1);
  std::unique_ptr<SerpMetrics> serp_metrics_1 = std::make_unique<SerpMetrics>(
      local_state(), SerpMetricsTimePeriodStoreFactory(
                         profile_path_1, profile_attributes_storage()));
  base::FilePath profile_path_2 =
      base::FilePath(kUserDataDir).AppendASCII("testing_profile_2");
  AddProfile(profile_path_2);
  std::unique_ptr<SerpMetrics> serp_metrics_2 = std::make_unique<SerpMetrics>(
      local_state(), SerpMetricsTimePeriodStoreFactory(
                         profile_path_2, profile_attributes_storage()));

  // Day 0: Stale
  serp_metrics_1->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_2->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_1->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_1->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_1->RecordSearch(SerpMetricType::kOther);
  serp_metrics_2->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_2->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_2->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_1->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_2->RecordSearch(SerpMetricType::kOther);

  SerpMetricsAllProfilesAggregator aggregator(local_state(),
                                              profile_attributes_storage());
  EXPECT_EQ(2U, aggregator.GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(2U, aggregator.GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(2U, aggregator.GetSearchCountForYesterday(SerpMetricType::kOther));
}

TEST_F(SerpMetricsAllProfilesAggregatorTest,
       AggregateStalePeriodRecordedMetricsForMultipleProfiles) {
  base::FilePath profile_path_1 =
      base::FilePath(kUserDataDir).AppendASCII("testing_profile_1");
  AddProfile(profile_path_1);
  std::unique_ptr<SerpMetrics> serp_metrics_1 = std::make_unique<SerpMetrics>(
      local_state(), SerpMetricsTimePeriodStoreFactory(
                         profile_path_1, profile_attributes_storage()));
  base::FilePath profile_path_2 =
      base::FilePath(kUserDataDir).AppendASCII("testing_profile_2");
  AddProfile(profile_path_2);
  std::unique_ptr<SerpMetrics> serp_metrics_2 = std::make_unique<SerpMetrics>(
      local_state(), SerpMetricsTimePeriodStoreFactory(
                         profile_path_2, profile_attributes_storage()));

  // Day 0: Stale
  serp_metrics_1->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_1->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_1->RecordSearch(SerpMetricType::kOther);
  serp_metrics_2->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_2->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_2->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  serp_metrics_1->RecordSearch(SerpMetricType::kBrave);
  serp_metrics_2->RecordSearch(SerpMetricType::kGoogle);
  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics_1->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics_2->RecordSearch(SerpMetricType::kOther);

  SerpMetricsAllProfilesAggregator aggregator(local_state(),
                                              profile_attributes_storage());
  EXPECT_EQ(6U, aggregator.GetSearchCountForStalePeriod());
}

}  // namespace serp_metrics

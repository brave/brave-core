/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_migration.h"

#include <memory>
#include <utility>

#include "base/check_deref.h"
#include "base/files/file_path.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/browser/serp_metrics/serp_metrics_time_period_store_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/time_period_storage/pref_time_period_store_factory.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

namespace {

constexpr base::FilePath::CharType kUserDataDir[] =
    FILE_PATH_LITERAL("/testing_user_data_dir");

std::unique_ptr<SerpMetrics> CreateProfilePrefsSerpMetrics(
    PrefService* local_state,
    PrefService* prefs) {
  return std::make_unique<SerpMetrics>(
      local_state,
      PrefTimePeriodStoreFactory(
          prefs, prefs::kDeprecatedSerpMetricsTimePeriodStorage.data()),
      /*report_in_utc=*/false);
}

}  // namespace

class SerpMetricsMigrationTest : public testing::Test {
 public:
  SerpMetricsMigrationTest()
      : profile_path_(
            base::FilePath(kUserDataDir).AppendASCII("testing_profile")) {}
  ~SerpMetricsMigrationTest() override = default;

  void SetUp() override {
    local_state_.registry()->RegisterStringPref(kLastCheckYMD,
                                                "");  // Never checked.
    profile_prefs_.registry()->RegisterDictionaryPref(
        prefs::kDeprecatedSerpMetricsTimePeriodStorage);
    ProfileAttributesStorage::RegisterPrefs(local_state_.registry());

    storage_ = std::make_unique<ProfileAttributesStorage>(
        &local_state_, base::FilePath(kUserDataDir));
    ProfileAttributesInitParams profile_init_params;
    profile_init_params.profile_path = profile_path_;
    storage_->AddProfile(std::move(profile_init_params));
  }

  const base::FilePath& profile_path() const { return profile_path_; }

  PrefService* local_state() { return &local_state_; }

  PrefService& profile_prefs() { return profile_prefs_; }

  ProfileAttributesEntry& profile_attributes_entry() {
    return CHECK_DEREF(storage_->GetProfileAttributesWithPath(profile_path_));
  }

  ProfileAttributesStorage& profile_attributes_storage() { return *storage_; }

  void AdvanceClockToNextDay() {
    task_environment_.AdvanceClock(base::Days(1));
  }

 private:
  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  const base::FilePath profile_path_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  std::unique_ptr<ProfileAttributesStorage> storage_;
};

TEST_F(SerpMetricsMigrationTest, MigrateFromEmptySerpMetrics) {
  MaybeMigrateSerpMetricsToProfileAttributes(profile_prefs(),
                                             profile_attributes_entry());

  std::unique_ptr<SerpMetrics> serp_metrics = std::make_unique<SerpMetrics>(
      local_state(),
      SerpMetricsTimePeriodStoreFactory(profile_path(),
                                        profile_attributes_storage()),
      /*report_in_utc=*/false);
  ASSERT_TRUE(serp_metrics);

  EXPECT_EQ(0U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(0U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(0U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kOther));

  EXPECT_EQ(0U, serp_metrics->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsMigrationTest, MigrateFromNonEmptySerpMetrics) {
  std::unique_ptr<SerpMetrics> prefs_serp_metrics =
      CreateProfilePrefsSerpMetrics(local_state(), &profile_prefs());
  ASSERT_TRUE(prefs_serp_metrics);

  // Day 0: Stale
  prefs_serp_metrics->RecordSearch(SerpMetricType::kBrave);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  prefs_serp_metrics->RecordSearch(SerpMetricType::kBrave);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 2: Today
  prefs_serp_metrics->RecordSearch(SerpMetricType::kBrave);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kOther);

  MaybeMigrateSerpMetricsToProfileAttributes(profile_prefs(),
                                             profile_attributes_entry());
  // Verify that the deprecated preference was cleared.
  EXPECT_TRUE(profile_prefs()
                  .GetDict(prefs::kDeprecatedSerpMetricsTimePeriodStorage)
                  .empty());

  std::unique_ptr<SerpMetrics> serp_metrics = std::make_unique<SerpMetrics>(
      local_state(),
      SerpMetricsTimePeriodStoreFactory(profile_path(),
                                        profile_attributes_storage()),
      /*report_in_utc=*/false);
  ASSERT_TRUE(serp_metrics);

  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kOther));

  EXPECT_EQ(3U, serp_metrics->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsMigrationTest,
       MigrateFromNonEmptySerpMetricsAndRecordNextDay) {
  std::unique_ptr<SerpMetrics> prefs_serp_metrics =
      CreateProfilePrefsSerpMetrics(local_state(), &profile_prefs());
  ASSERT_TRUE(prefs_serp_metrics);

  // Day 0: Stale
  prefs_serp_metrics->RecordSearch(SerpMetricType::kBrave);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  prefs_serp_metrics->RecordSearch(SerpMetricType::kBrave);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kOther);

  MaybeMigrateSerpMetricsToProfileAttributes(profile_prefs(),
                                             profile_attributes_entry());
  // Verify that the deprecated preference was cleared.
  EXPECT_TRUE(profile_prefs()
                  .GetDict(prefs::kDeprecatedSerpMetricsTimePeriodStorage)
                  .empty());

  std::unique_ptr<SerpMetrics> serp_metrics = std::make_unique<SerpMetrics>(
      local_state(),
      SerpMetricsTimePeriodStoreFactory(profile_path(),
                                        profile_attributes_storage()),
      /*report_in_utc=*/false);
  ASSERT_TRUE(serp_metrics);

  AdvanceClockToNextDay();

  // Day 2: Today
  serp_metrics->RecordSearch(SerpMetricType::kBrave);
  serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  serp_metrics->RecordSearch(SerpMetricType::kOther);

  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(3U, serp_metrics->GetSearchCountForStalePeriod());
}

TEST_F(SerpMetricsMigrationTest, DoubleMigrationIsNoOp) {
  std::unique_ptr<SerpMetrics> prefs_serp_metrics =
      CreateProfilePrefsSerpMetrics(local_state(), &profile_prefs());
  ASSERT_TRUE(prefs_serp_metrics);

  // Day 0: Stale
  prefs_serp_metrics->RecordSearch(SerpMetricType::kBrave);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  // Day 1: Yesterday
  prefs_serp_metrics->RecordSearch(SerpMetricType::kBrave);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kGoogle);
  prefs_serp_metrics->RecordSearch(SerpMetricType::kOther);
  AdvanceClockToNextDay();

  MaybeMigrateSerpMetricsToProfileAttributes(profile_prefs(),
                                             profile_attributes_entry());
  // Verify that the deprecated preference was cleared.
  EXPECT_TRUE(profile_prefs()
                  .GetDict(prefs::kDeprecatedSerpMetricsTimePeriodStorage)
                  .empty());

  std::unique_ptr<SerpMetrics> serp_metrics = std::make_unique<SerpMetrics>(
      local_state(),
      SerpMetricsTimePeriodStoreFactory(profile_path(),
                                        profile_attributes_storage()),
      /*report_in_utc=*/false);
  ASSERT_TRUE(serp_metrics);

  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(3U, serp_metrics->GetSearchCountForStalePeriod());

  // Double migration does nothing.
  MaybeMigrateSerpMetricsToProfileAttributes(profile_prefs(),
                                             profile_attributes_entry());

  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kBrave));
  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kGoogle));
  EXPECT_EQ(1U,
            serp_metrics->GetSearchCountForYesterday(SerpMetricType::kOther));
  EXPECT_EQ(3U, serp_metrics->GetSearchCountForStalePeriod());
}

}  // namespace serp_metrics

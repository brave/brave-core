/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/time_period_events_metric.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {
constexpr char kTestHistogramName[] = "TestHistogram";
constexpr char kTestStorageKey[] = "test_storage_key";

constexpr char kTestMetricDefinitionJson[] = R"({
  "histogram_name": "TestHistogram",
  "storage_key": "test_storage_key",
  "period_days": 7,
  "buckets": [5, 10, 20]
})";

constexpr char kTestMetricReportMaxJson[] = R"({
  "histogram_name": "TestHistogram",
  "storage_key": "test_storage_key",
  "period_days": 7,
  "buckets": [5, 10, 20],
  "report_max": true,
  "add_histogram_value_to_storage": true
})";

constexpr char kTestMetricAddHistogramValueJson[] = R"({
  "histogram_name": "TestHistogram",
  "storage_key": "test_storage_key",
  "period_days": 7,
  "buckets": [5, 10, 20],
  "add_histogram_value_to_storage": true
})";

// Parses TimePeriodEventsMetricDefinition from JSON
TimePeriodEventsMetricDefinition ParseMetricDefinition(std::string_view json) {
  base::JSONValueConverter<TimePeriodEventsMetricDefinition> converter;
  auto dict = base::JSONReader::Read(json);
  CHECK(dict.has_value());

  TimePeriodEventsMetricDefinition definition;
  converter.Convert(*dict, &definition);

  return definition;
}
}  // namespace

class P3ATimePeriodEventsMetricUnitTest : public testing::Test {
 public:
  P3ATimePeriodEventsMetricUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    local_state_.registry()->RegisterDictionaryPref(
        kRemoteMetricStorageDictPref);
  }

  void CreateMetric(std::string_view json_definition) {
    auto definition = ParseMetricDefinition(json_definition);

    metric_ = std::make_unique<TimePeriodEventsMetric>(
        &local_state_, std::move(definition),
        base::BindRepeating(&P3ATimePeriodEventsMetricUnitTest::OnMetricUpdated,
                            base::Unretained(this)));
  }

  void OnMetricUpdated(size_t value) {
    last_reported_value_ = value;
    report_count_++;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<TimePeriodEventsMetric> metric_;

  size_t last_reported_value_ = 0;
  size_t report_count_ = 0;
};

TEST_F(P3ATimePeriodEventsMetricUnitTest, ValidateDefinition) {
  auto valid_def = ParseMetricDefinition(kTestMetricDefinitionJson);
  EXPECT_TRUE(valid_def.Validate());

  // Create invalid definition with no buckets
  const auto* invalid_json1 = R"({
    "histogram_name": "TestHistogram",
    "storage_key": "test_storage_key",
    "period_days": 7,
    "buckets": []
  })";
  auto invalid_def1 = ParseMetricDefinition(invalid_json1);
  EXPECT_FALSE(invalid_def1.Validate());

  // Create invalid definition with period_days = 0
  const auto* invalid_json2 = R"({
    "histogram_name": "TestHistogram",
    "storage_key": "test_storage_key",
    "period_days": 0,
    "buckets": [5]
  })";
  auto invalid_def2 = ParseMetricDefinition(invalid_json2);
  EXPECT_FALSE(invalid_def2.Validate());

  // Create invalid definition with empty histogram_name
  const auto* invalid_json3 = R"({
    "histogram_name": "",
    "storage_key": "test_storage_key",
    "period_days": 7,
    "buckets": [5]
  })";
  auto invalid_def3 = ParseMetricDefinition(invalid_json3);
  EXPECT_FALSE(invalid_def3.Validate());

  // Create invalid definition with empty storage_key
  const auto* invalid_json4 = R"({
    "histogram_name": "TestHistogram",
    "storage_key": "",
    "period_days": 7,
    "buckets": [5]
  })";
  auto invalid_def4 = ParseMetricDefinition(invalid_json4);
  EXPECT_FALSE(invalid_def4.Validate());
}

TEST_F(P3ATimePeriodEventsMetricUnitTest, GetSourceHistogramNames) {
  CreateMetric(kTestMetricDefinitionJson);

  auto histogram_names = metric_->GetSourceHistogramNames();
  ASSERT_EQ(histogram_names.size(), 1u);
  EXPECT_EQ(histogram_names[0], kTestHistogramName);
}

TEST_F(P3ATimePeriodEventsMetricUnitTest, GetStorageKey) {
  CreateMetric(kTestMetricDefinitionJson);

  auto storage_key = metric_->GetStorageKey();
  ASSERT_TRUE(storage_key.has_value());
  EXPECT_EQ(storage_key.value(), kTestStorageKey);
}

TEST_F(P3ATimePeriodEventsMetricUnitTest, HandleHistogramChange) {
  CreateMetric(kTestMetricDefinitionJson);

  EXPECT_EQ(report_count_, 1u);
  EXPECT_EQ(last_reported_value_, 0u);

  metric_->HandleHistogramChange(kTestHistogramName, 1);

  EXPECT_EQ(report_count_, 2u);
  EXPECT_EQ(last_reported_value_, 0u);

  for (size_t i = 0; i < 5; i++) {
    metric_->HandleHistogramChange(kTestHistogramName, 1);
  }

  EXPECT_EQ(report_count_, 7u);
  EXPECT_EQ(last_reported_value_, 1u);

  for (size_t i = 0; i < 5; i++) {
    metric_->HandleHistogramChange(kTestHistogramName, 1);
  }

  EXPECT_EQ(report_count_, 12u);
  EXPECT_EQ(last_reported_value_, 2u);
}

TEST_F(P3ATimePeriodEventsMetricUnitTest, ReportMax) {
  CreateMetric(kTestMetricReportMaxJson);

  EXPECT_EQ(report_count_, 1u);
  EXPECT_EQ(last_reported_value_, 0u);

  metric_->HandleHistogramChange(kTestHistogramName, 3);
  EXPECT_EQ(last_reported_value_, 0u);

  metric_->HandleHistogramChange(kTestHistogramName, 7);
  EXPECT_EQ(last_reported_value_, 1u);

  metric_->HandleHistogramChange(kTestHistogramName, 2);
  EXPECT_EQ(last_reported_value_, 1u);

  metric_->HandleHistogramChange(kTestHistogramName, 15);
  EXPECT_EQ(last_reported_value_, 2u);

  metric_->HandleHistogramChange(kTestHistogramName, 6);
  EXPECT_EQ(last_reported_value_, 2u);
}

TEST_F(P3ATimePeriodEventsMetricUnitTest, AddHistogramValue) {
  CreateMetric(kTestMetricAddHistogramValueJson);

  EXPECT_EQ(report_count_, 1u);
  EXPECT_EQ(last_reported_value_, 0u);

  metric_->HandleHistogramChange(kTestHistogramName, 3);
  EXPECT_EQ(last_reported_value_, 0u);

  metric_->HandleHistogramChange(kTestHistogramName, 4);
  EXPECT_EQ(last_reported_value_, 1u);

  metric_->HandleHistogramChange(kTestHistogramName, 6);
  EXPECT_EQ(last_reported_value_, 2u);

  task_environment_.FastForwardBy(base::Days(2));
  metric_->HandleHistogramChange(kTestHistogramName, 2);
  EXPECT_EQ(last_reported_value_, 2u);

  metric_->HandleHistogramChange(kTestHistogramName, 10);
  EXPECT_EQ(last_reported_value_, 3u);
}

TEST_F(P3ATimePeriodEventsMetricUnitTest, PeriodRollover) {
  CreateMetric(kTestMetricDefinitionJson);

  for (size_t i = 0; i < 6; i++) {
    metric_->HandleHistogramChange(kTestHistogramName, 1);
  }
  EXPECT_EQ(report_count_, 7u);
  EXPECT_EQ(last_reported_value_, 1u);

  task_environment_.FastForwardBy(base::Days(6));
  EXPECT_EQ(report_count_, 13u);
  EXPECT_EQ(last_reported_value_, 1u);

  task_environment_.FastForwardBy(base::Days(1));

  EXPECT_EQ(report_count_, 14u);
  EXPECT_EQ(last_reported_value_, 0u);
}

}  // namespace p3a

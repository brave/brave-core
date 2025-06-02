/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/pref_metric.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {
constexpr char kTestPrefName[] = "test_pref";

constexpr char kPrefMetricJson[] = R"({
  "pref_name": "%s",
  "value_map": {
    "option1": 0,
    "option2": 1,
    "option3": 2
  },
  "use_profile_prefs": %s
})";

PrefMetricDefinition ParseMetricDefinition(std::string_view json) {
  base::JSONValueConverter<PrefMetricDefinition> converter;
  auto dict = base::JSONReader::Read(json);
  CHECK(dict.has_value());

  PrefMetricDefinition definition;
  converter.Convert(*dict, &definition);

  return definition;
}

}  // namespace

class P3APrefMetricUnitTest : public testing::Test,
                              public RemoteMetric::Delegate {
 public:
  P3APrefMetricUnitTest() = default;

  void SetUp() override {
    local_state_.registry()->RegisterStringPref(kTestPrefName, "option1");
    primary_profile_prefs_.registry()->RegisterStringPref(kTestPrefName,
                                                          "option1");
    secondary_profile_prefs_.registry()->RegisterStringPref(kTestPrefName,
                                                            "option3");
  }

  void CreateMetric(bool use_profile_prefs,
                    std::string_view pref_name = kTestPrefName) {
    auto json = base::StringPrintf(kPrefMetricJson, pref_name,
                                   use_profile_prefs ? "true" : "false");
    auto definition = ParseMetricDefinition(json);
    metric_ = std::make_unique<PrefMetric>(&local_state_, std::move(definition),
                                           this, "test_metric");
  }

  // RemoteMetric::Delegate:
  void UpdateMetric(std::string_view metric_name, size_t bucket) override {
    last_reported_value_ = bucket;
    report_count_++;
  }

  TimePeriodStorage* GetTimePeriodStorage(std::string_view storage_key,
                                          int period_days) override {
    return nullptr;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple primary_profile_prefs_;
  TestingPrefServiceSimple secondary_profile_prefs_;
  std::unique_ptr<PrefMetric> metric_;

  size_t last_reported_value_ = 0;
  size_t report_count_ = 0;
};

TEST_F(P3APrefMetricUnitTest, ValidateDefinition) {
  const auto* valid_json = R"({
    "histogram_name": "TestHistogram",
    "pref_name": "test_pref",
    "use_profile_prefs": false,
    "value_map": {
      "option1": 0,
      "option2": 1,
      "option3": 2
    }
  })";
  auto valid_def = ParseMetricDefinition(valid_json);
  EXPECT_TRUE(valid_def.Validate());

  const auto* invalid_json1 = R"({
    "histogram_name": "TestHistogram",
    "pref_name": "",
    "use_profile_prefs": false,
    "value_map": {
      "option1": 0,
      "option2": 1,
      "option3": 2
    }
  })";
  auto invalid_def1 = ParseMetricDefinition(invalid_json1);
  EXPECT_FALSE(invalid_def1.Validate());

  const auto* invalid_json2 = R"({
    "histogram_name": "TestHistogram",
    "pref_name": "test_pref",
    "use_profile_prefs": false,
    "value_map": {}
  })";
  auto invalid_def2 = ParseMetricDefinition(invalid_json2);
  EXPECT_FALSE(invalid_def2.Validate());
}

TEST_F(P3APrefMetricUnitTest, EmptyHistogramAndStorageKey) {
  CreateMetric(false);

  auto histogram_names = metric_->GetSourceHistogramNames();
  EXPECT_TRUE(histogram_names.empty());

  auto storage_keys = metric_->GetStorageKeys();
  EXPECT_FALSE(storage_keys.has_value());
}

TEST_F(P3APrefMetricUnitTest, LocalStateBasics) {
  CreateMetric(false);

  EXPECT_EQ(report_count_, 1u);
  EXPECT_EQ(last_reported_value_, 0u);

  local_state_.SetString(kTestPrefName, "option2");
  EXPECT_EQ(report_count_, 2u);
  EXPECT_EQ(last_reported_value_, 1u);

  local_state_.SetString(kTestPrefName, "unknown_option");
  EXPECT_EQ(report_count_, 2u);

  local_state_.SetString(kTestPrefName, "option3");
  EXPECT_EQ(report_count_, 3u);
  EXPECT_EQ(last_reported_value_, 2u);
}

TEST_F(P3APrefMetricUnitTest, ProfilePrefsBasics) {
  CreateMetric(true);

  EXPECT_EQ(report_count_, 0u);

  metric_->OnLastUsedProfilePrefsChanged(&primary_profile_prefs_);
  EXPECT_EQ(report_count_, 1u);
  EXPECT_EQ(last_reported_value_, 0u);

  primary_profile_prefs_.SetString(kTestPrefName, "option2");
  EXPECT_EQ(report_count_, 2u);
  EXPECT_EQ(last_reported_value_, 1u);

  metric_->OnLastUsedProfilePrefsChanged(&secondary_profile_prefs_);
  EXPECT_EQ(report_count_, 3u);
  EXPECT_EQ(last_reported_value_, 2u);

  secondary_profile_prefs_.SetString(kTestPrefName, "option1");
  EXPECT_EQ(report_count_, 4u);
  EXPECT_EQ(last_reported_value_, 0u);

  secondary_profile_prefs_.SetString(kTestPrefName, "option3");
  EXPECT_EQ(report_count_, 5u);
  EXPECT_EQ(last_reported_value_, 2u);
}

TEST_F(P3APrefMetricUnitTest, NonExistentPref) {
  CreateMetric(false, "non_existent_pref");

  EXPECT_EQ(report_count_, 0u);
}

}  // namespace p3a

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/remote_metric_manager.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/p3a/pref_names.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr char kTestHistogramName[] = "TestHistogram";
constexpr char kTestPrefName[] = "test_pref";
constexpr char kTestMetricName[] = "test_metric";
constexpr char kTestStorageKey[] = "test_storage_key";

constexpr char kTimePeriodEventsMetricJson[] = R"({
  "type": "time_period_events",
  "histogram_name": "TestHistogram",
  "storage_key": "test_storage_key",
  "period_days": 7,
  "buckets": [5, 10, 20]
})";

constexpr char kPrefMetricJson[] = R"({
  "type": "pref",
  "pref_name": "test_pref",
  "value_map": {
    "option1": 0,
    "option2": 1,
    "option3": 2
  },
  "use_profile_prefs": true
})";

constexpr char kTimePeriodEventsMetricWithFutureMinVersionJson[] = R"({
  "type": "time_period_events",
  "histogram_name": "TestHistogram2",
  "storage_key": "test_storage_key_future",
  "period_days": 7,
  "buckets": [5, 10, 20],
  "min_version": "999.0.0"
})";

constexpr char kPrefMetricWithValidMinVersionJson[] = R"({
  "type": "pref",
  "pref_name": "test_pref",
  "value_map": {
    "option1": 0,
    "option2": 1
  },
  "use_profile_prefs": true,
  "min_version": "1.0.0"
})";

}  // namespace

class P3ARemoteMetricManagerUnitTest : public testing::Test,
                                       public RemoteMetricManager::Delegate {
 public:
  P3ARemoteMetricManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    local_state_.registry()->RegisterDictionaryPref(
        kRemoteMetricStorageDictPref);
    local_state_.registry()->RegisterFilePathPref(::prefs::kProfileLastUsed,
                                                  {});
    local_state_.registry()->RegisterStringPref(kTestPrefName, "option1");

    primary_profile_path_ = base::FilePath(FILE_PATH_LITERAL("profile1"));
    secondary_profile_path_ = base::FilePath(FILE_PATH_LITERAL("profile2"));

    primary_profile_prefs_.registry()->RegisterStringPref(kTestPrefName,
                                                          "option1");
    secondary_profile_prefs_.registry()->RegisterStringPref(kTestPrefName,
                                                            "option3");

    manager_ = std::make_unique<RemoteMetricManager>(&local_state_, this);
  }

  // RemoteMetricManager::Delegate implementation
  void UpdateMetricValue(
      std::string_view histogram_name,
      size_t bucket,
      std::optional<bool> only_update_for_constellation) override {
    last_updated_metric_ = std::string(histogram_name);
    last_updated_bucket_ = bucket;
    update_count_++;
  }

 protected:
  void AddDefinition(RemoteMetricManager::UnparsedDefinitionsMap& definitions,
                     const std::string& metric_name,
                     std::string_view json) {
    auto definition_value = base::JSONReader::Read(json);
    ASSERT_TRUE(definition_value.has_value());
    definitions[metric_name] = std::make_unique<base::Value::Dict>(
        definition_value->GetDict().Clone());
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple primary_profile_prefs_;
  TestingPrefServiceSimple secondary_profile_prefs_;
  std::unique_ptr<RemoteMetricManager> manager_;

  base::FilePath primary_profile_path_;
  base::FilePath secondary_profile_path_;

  std::string last_updated_metric_;
  size_t last_updated_bucket_ = 0;
  size_t update_count_ = 0;
};

TEST_F(P3ARemoteMetricManagerUnitTest, ProcessMetricDefinitions) {
  RemoteMetricManager::UnparsedDefinitionsMap definitions;

  AddDefinition(definitions, kTestMetricName, kTimePeriodEventsMetricJson);

  manager_->ProcessMetricDefinitions(definitions);

  EXPECT_EQ(manager_->metrics_.size(), 1u);
  EXPECT_EQ(manager_->histogram_to_metrics_.size(), 1u);
  EXPECT_TRUE(manager_->histogram_to_metrics_.contains(kTestHistogramName));
  EXPECT_EQ(manager_->histogram_to_metrics_[kTestHistogramName].size(), 1u);

  definitions.clear();
  AddDefinition(definitions, "metric1", kTimePeriodEventsMetricJson);
  AddDefinition(definitions, "metric2", kPrefMetricJson);

  manager_->ProcessMetricDefinitions(definitions);

  EXPECT_EQ(manager_->metrics_.size(), 2u);
  EXPECT_EQ(manager_->histogram_to_metrics_.size(), 1u);
  EXPECT_TRUE(manager_->histogram_to_metrics_.contains(kTestHistogramName));
  EXPECT_EQ(manager_->histogram_to_metrics_[kTestHistogramName].size(), 1u);
}

TEST_F(P3ARemoteMetricManagerUnitTest, InvalidMetricDefinitionsAreSkipped) {
  RemoteMetricManager::UnparsedDefinitionsMap definitions;

  // Add a valid metric
  AddDefinition(definitions, "valid_metric", kTimePeriodEventsMetricJson);

  // Add an invalid metric missing the type
  auto invalid_type = std::make_unique<base::Value::Dict>();
  invalid_type->Set("histogram_name", kTestHistogramName);
  definitions["invalid_type"] = std::move(invalid_type);

  // Add an invalid metric with unknown type
  auto invalid_unknown_type = std::make_unique<base::Value::Dict>();
  invalid_unknown_type->Set("type", "unknown_type");
  definitions["invalid_unknown_type"] = std::move(invalid_unknown_type);

  // Add an invalid metric with invalid time period events definition
  auto invalid_tpe = std::make_unique<base::Value::Dict>();
  invalid_tpe->Set("type", "time_period_events");
  definitions["invalid_tpe"] = std::move(invalid_tpe);

  manager_->ProcessMetricDefinitions(definitions);

  // Only the valid metric should be processed
  EXPECT_EQ(manager_->metrics_.size(), 1u);
}

TEST_F(P3ARemoteMetricManagerUnitTest, ProfilePrefsHandling) {
  RemoteMetricManager::UnparsedDefinitionsMap definitions;

  EXPECT_EQ(manager_->last_used_profile_prefs_, nullptr);

  local_state_.SetFilePath(::prefs::kProfileLastUsed, primary_profile_path_);

  manager_->HandleProfileLoad(&primary_profile_prefs_, primary_profile_path_);

  EXPECT_EQ(manager_->last_used_profile_prefs_, &primary_profile_prefs_);

  manager_->HandleProfileLoad(&secondary_profile_prefs_,
                              secondary_profile_path_);

  EXPECT_EQ(manager_->last_used_profile_prefs_, &primary_profile_prefs_);

  local_state_.SetFilePath(::prefs::kProfileLastUsed, secondary_profile_path_);

  EXPECT_EQ(manager_->last_used_profile_prefs_, &secondary_profile_prefs_);

  manager_->HandleProfileUnload(secondary_profile_path_);

  EXPECT_EQ(manager_->last_used_profile_prefs_, nullptr);
}

TEST_F(P3ARemoteMetricManagerUnitTest, CleanupStorage) {
  RemoteMetricManager::UnparsedDefinitionsMap definitions;

  AddDefinition(definitions, kTestMetricName, kTimePeriodEventsMetricJson);

  manager_->ProcessMetricDefinitions(definitions);

  base::Value::Dict dict;
  dict.Set(kTestStorageKey, base::Value::Dict());
  dict.Set("unused_key", base::Value::Dict());
  local_state_.SetDict(kRemoteMetricStorageDictPref, std::move(dict));

  definitions.clear();
  manager_->ProcessMetricDefinitions(definitions);

  const auto& storage = local_state_.GetDict(kRemoteMetricStorageDictPref);
  EXPECT_FALSE(storage.FindDict("unused_key"));
  EXPECT_FALSE(storage.FindDict(kTestStorageKey));
}

TEST_F(P3ARemoteMetricManagerUnitTest, MinVersionAccepted) {
  // Set current version to a version that should accept min_version 1.0.0
  manager_->current_version_ = base::Version("1.70.0");

  RemoteMetricManager::UnparsedDefinitionsMap definitions;

  // Add a metric with valid min_version for pref type (should be accepted)
  AddDefinition(definitions, "metric_valid_pref",
                kPrefMetricWithValidMinVersionJson);

  // Add a metric without min_version (should be accepted)
  AddDefinition(definitions, "metric_no_version", kTimePeriodEventsMetricJson);

  manager_->ProcessMetricDefinitions(definitions);

  // Should create all 2 metrics since they meet version requirements
  EXPECT_EQ(manager_->metrics_.size(), 2u);

  // Verify histogram mappings
  EXPECT_EQ(manager_->histogram_to_metrics_.size(), 1u);
  EXPECT_TRUE(manager_->histogram_to_metrics_.contains(kTestHistogramName));
  EXPECT_EQ(manager_->histogram_to_metrics_[kTestHistogramName].size(), 1u);
}

TEST_F(P3ARemoteMetricManagerUnitTest, MinVersionRejected) {
  // Set current version to a low version that should reject future versions
  manager_->current_version_ = base::Version("1.0.0");

  RemoteMetricManager::UnparsedDefinitionsMap definitions;

  // Add a metric with future min_version (should be rejected)
  AddDefinition(definitions, "metric_future_version",
                kTimePeriodEventsMetricWithFutureMinVersionJson);

  manager_->ProcessMetricDefinitions(definitions);

  // Should create no metrics since the future version should be rejected
  EXPECT_TRUE(manager_->metrics_.empty());
  EXPECT_TRUE(manager_->histogram_to_metrics_.empty());
}

}  // namespace p3a

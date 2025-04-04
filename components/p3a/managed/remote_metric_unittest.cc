/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/remote_metric.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {
constexpr char kTestMetricName[] = "test_remote_metric";
constexpr char kNumeratorPrefName[] = "numerator_pref";
constexpr char kDenominatorPrefName[] = "denominator_pref";

constexpr char kMixedPrefsDefinitionJson[] = R"({
  "type": "bucket",
  "source": {
    "type": "percentage",
    "numerator": {
      "type": "pref",
      "pref_name": "numerator_pref",
      "use_profile_prefs": false
    },
    "denominator": {
      "type": "pref",
      "pref_name": "denominator_pref",
      "use_profile_prefs": true
    }
  },
  "buckets": [10, 20, 30, 50]
})";

constexpr char kDefinitionWithMinVersionJson[] = R"({
  "type": "bucket",
  "min_version": "2.0.0",
  "source": {
    "type": "percentage",
    "numerator": {
      "type": "pref",
      "pref_name": "numerator_pref",
      "use_profile_prefs": false
    },
    "denominator": {
      "type": "pref",
      "pref_name": "denominator_pref",
      "use_profile_prefs": true
    }
  },
  "buckets": [10, 20, 30, 50]
})";
}  // namespace

class P3ARemoteMetricTest : public testing::Test,
                            public RemoteMetric::Delegate {
 public:
  P3ARemoteMetricTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    // Register numerator pref on local state, denominator pref on profile prefs
    local_state_.registry()->RegisterIntegerPref(kNumeratorPrefName, 30);
    profile_prefs_.registry()->RegisterIntegerPref(kDenominatorPrefName, 120);
  }

  // RemoteMetric::Delegate implementation
  void UpdateMetric(std::string_view metric_name, size_t bucket) override {
    if (metric_name != kTestMetricName) {
      return;
    }
    last_updated_bucket_ = bucket;
    update_count_++;
  }

  TimePeriodStorage* GetTimePeriodStorage(std::string_view storage_key,
                                          int period_days) override {
    return nullptr;
  }

 protected:
  std::unique_ptr<RemoteMetric> CreateRemoteMetric(
      std::string_view json,
      std::optional<std::string> version_str = std::nullopt) {
    auto definition_value = base::JSONReader::Read(json);
    EXPECT_TRUE(definition_value.has_value());
    if (!definition_value) {
      return nullptr;
    }
    auto metric = std::make_unique<RemoteMetric>(
        &local_state_, &profile_prefs_, this, kTestMetricName,
        std::make_unique<base::Value>(std::move(*definition_value)));

    base::Version current_version(version_str.value_or("1.0.0"));
    if (metric->Init(current_version)) {
      return metric;
    }
    return nullptr;
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;

  size_t last_updated_bucket_ = 0;
  size_t update_count_ = 0;
};

TEST_F(P3ARemoteMetricTest, InitFailsWithInvalidDefinition) {
  // Test with invalid JSON
  auto metric1 = CreateRemoteMetric(R"({"type": "invalid"})");
  EXPECT_FALSE(metric1);

  // Test with version less than required 2.0.0
  auto metric2 = CreateRemoteMetric(kDefinitionWithMinVersionJson, "1.5.0");
  EXPECT_FALSE(metric2);
}

TEST_F(P3ARemoteMetricTest, InitSucceedsWithValidMinVersion) {
  // Test with version greater than required 2.0.0
  auto metric = CreateRemoteMetric(kDefinitionWithMinVersionJson, "2.1.0");
  EXPECT_TRUE(metric);
}

TEST_F(P3ARemoteMetricTest, ProcessNestedIntermediatesWithMixedPrefs) {
  auto metric = CreateRemoteMetric(kMixedPrefsDefinitionJson);
  ASSERT_TRUE(metric);

  // Initial values: numerator=30 (local), denominator=120 (profile) -> 25% ->
  // bucket 2 (between 20 and 30)
  EXPECT_EQ(update_count_, 1u);
  EXPECT_EQ(last_updated_bucket_, 2u);

  // Fast forward time to trigger daily update
  task_environment_.FastForwardBy(base::Days(1));

  EXPECT_EQ(update_count_, 2u);
  EXPECT_EQ(last_updated_bucket_, 2u);

  local_state_.SetInteger(kNumeratorPrefName,
                          5);  // 5/120 = 4.2% -> bucket 0 (below 10)

  EXPECT_EQ(update_count_, 3u);
  EXPECT_EQ(last_updated_bucket_, 0u);

  profile_prefs_.SetInteger(kDenominatorPrefName,
                            10);  // 5/10 = 50% -> bucket 3

  EXPECT_EQ(update_count_, 4u);
  EXPECT_EQ(last_updated_bucket_, 3u);

  task_environment_.FastForwardBy(base::Days(1));

  EXPECT_EQ(update_count_, 5u);
  EXPECT_EQ(last_updated_bucket_, 3u);
}

TEST_F(P3ARemoteMetricTest, OnLastUsedProfilePrefsChanged) {
  auto metric = CreateRemoteMetric(kMixedPrefsDefinitionJson);
  ASSERT_TRUE(metric);

  EXPECT_EQ(update_count_, 1u);
  EXPECT_EQ(last_updated_bucket_, 2u);

  metric->OnLastUsedProfilePrefsChanged(&profile_prefs_);

  EXPECT_EQ(update_count_, 2u);
  EXPECT_EQ(last_updated_bucket_, 2u);
}

}  // namespace p3a

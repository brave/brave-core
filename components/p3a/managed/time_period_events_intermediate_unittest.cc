/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/time_period_events_intermediate.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

constexpr char kTestTimePeriodPref[] = "test_key";

class P3ATimePeriodEventsIntermediateTest
    : public testing::Test,
      public RemoteMetricIntermediate::Delegate {
 public:
  class MockIntermediate : public RemoteMetricIntermediate {
   public:
    explicit MockIntermediate(std::string storage_key)
        : RemoteMetricIntermediate(nullptr), storage_key_(storage_key) {}
    ~MockIntermediate() override = default;

    bool Init() override { return true; }
    base::Value Process() override { return value_.Clone(); }
    base::flat_set<std::string_view> GetStorageKeys() const override {
      return {{storage_key_}};
    }
    void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) override {}

    base::Value value_;

   private:
    std::string storage_key_;
  };

  P3ATimePeriodEventsIntermediateTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    pref_service_.registry()->RegisterListPref(kTestTimePeriodPref);
  }

  // RemoteMetricIntermediate::Delegate implementation
  void TriggerUpdate() override {}
  TimePeriodStorage* GetTimePeriodStorage(std::string_view key,
                                          int period_days) override {
    if (key == kTestTimePeriodPref) {
      if (!storage_) {
        storage_ = std::make_unique<TimePeriodStorage>(
            &pref_service_, kTestTimePeriodPref, nullptr, period_days);
      }
      return storage_.get();
    }
    return nullptr;
  }
  std::unique_ptr<RemoteMetricIntermediate> GetIntermediateInstance(
      const base::Value& source) override {
    if (source.is_string() && source.GetString() == "mock_source1") {
      auto mock_intermediate = std::make_unique<MockIntermediate>("mock_key1");
      mock_intermediate1_ = mock_intermediate.get();
      return mock_intermediate;
    }
    if (source.is_string() && source.GetString() == "mock_source2") {
      auto mock_intermediate = std::make_unique<MockIntermediate>("mock_key2");
      mock_intermediate2_ = mock_intermediate.get();
      return mock_intermediate;
    }
    return nullptr;
  }

 protected:
  TimePeriodEventsIntermediateDefinition ParseDefinition(
      std::string_view json) {
    auto definition_value = base::JSONReader::Read(json);
    EXPECT_TRUE(definition_value.has_value());

    TimePeriodEventsIntermediateDefinition definition;
    base::JSONValueConverter<TimePeriodEventsIntermediateDefinition> converter;
    EXPECT_TRUE(converter.Convert(definition_value->GetDict(), &definition));

    return definition;
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<TimePeriodStorage> storage_;
  std::unique_ptr<TimePeriodEventsIntermediate> events_;
  raw_ptr<MockIntermediate> mock_intermediate1_ = nullptr;
  raw_ptr<MockIntermediate> mock_intermediate2_ = nullptr;
};

TEST_F(P3ATimePeriodEventsIntermediateTest, InitFailsWithEmptyProperties) {
  const char json1[] = R"({
    "period_days": 28
  })";

  auto def1 = ParseDefinition(json1);
  EXPECT_TRUE(def1.storage_key.empty());
  EXPECT_EQ(def1.period_days, 28);

  events_ =
      std::make_unique<TimePeriodEventsIntermediate>(std::move(def1), this);
  EXPECT_FALSE(events_->Init());

  const char json2[] = R"({
    "storage_key": "test_key"
  })";

  auto def2 = ParseDefinition(json2);
  EXPECT_EQ(def2.storage_key, "test_key");
  EXPECT_EQ(def2.period_days, 0);

  events_ =
      std::make_unique<TimePeriodEventsIntermediate>(std::move(def2), this);
  EXPECT_FALSE(events_->Init());

  EXPECT_FALSE(mock_intermediate1_);
  EXPECT_FALSE(mock_intermediate2_);
}

TEST_F(P3ATimePeriodEventsIntermediateTest, ProcessStoresSourceValues) {
  const char json[] = R"({
    "storage_key": "test_key",
    "period_days": 28,
    "sources": ["mock_source1"]
  })";

  auto def = ParseDefinition(json);
  events_ =
      std::make_unique<TimePeriodEventsIntermediate>(std::move(def), this);
  ASSERT_TRUE(events_->Init());

  auto result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 0);

  ASSERT_TRUE(mock_intermediate1_);
  mock_intermediate1_->value_ = base::Value(42);

  result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 1);
}

TEST_F(P3ATimePeriodEventsIntermediateTest, ProcessWithMultipleSources) {
  const char json[] = R"({
    "storage_key": "test_key",
    "period_days": 28,
    "sources": ["mock_source1", "mock_source2"]
  })";

  auto def = ParseDefinition(json);
  events_ =
      std::make_unique<TimePeriodEventsIntermediate>(std::move(def), this);
  ASSERT_TRUE(events_->Init());

  ASSERT_TRUE(mock_intermediate1_);
  ASSERT_TRUE(mock_intermediate2_);

  mock_intermediate1_->value_ = base::Value(10);
  mock_intermediate2_->value_ = base::Value(20);

  auto result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 2);

  task_environment_.AdvanceClock(base::Days(1));

  mock_intermediate1_->value_ = {};
  mock_intermediate2_->value_ = base::Value(20);

  result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 3);
}

TEST_F(P3ATimePeriodEventsIntermediateTest, ProcessWithReportHighest) {
  const char json[] = R"({
    "storage_key": "test_key",
    "period_days": 28,
    "add_histogram_value": true,
    "report_highest": true,
    "sources": ["mock_source1", "mock_source2"]
  })";

  auto def = ParseDefinition(json);
  events_ =
      std::make_unique<TimePeriodEventsIntermediate>(std::move(def), this);
  ASSERT_TRUE(events_->Init());

  ASSERT_TRUE(mock_intermediate1_);
  ASSERT_TRUE(mock_intermediate2_);

  mock_intermediate1_->value_ = base::Value(10);

  auto result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 10);

  mock_intermediate1_->value_ = {};

  task_environment_.AdvanceClock(base::Days(1));

  mock_intermediate2_->value_ = base::Value(20);

  result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 20);
}

TEST_F(P3ATimePeriodEventsIntermediateTest, ProcessWithAddHistogramValue) {
  const char json[] = R"({
    "storage_key": "test_key",
    "period_days": 28,
    "add_histogram_value": true,
    "sources": ["mock_source1"]
  })";

  auto def = ParseDefinition(json);
  events_ =
      std::make_unique<TimePeriodEventsIntermediate>(std::move(def), this);
  ASSERT_TRUE(events_->Init());

  ASSERT_TRUE(mock_intermediate1_);

  mock_intermediate1_->value_ = base::Value(5);
  events_->Process();

  mock_intermediate1_->value_ = base::Value(10);
  auto result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 15);
}

TEST_F(P3ATimePeriodEventsIntermediateTest, ProcessWithTimePeriodBehavior) {
  const char json[] = R"({
    "storage_key": "test_key",
    "period_days": 7,
    "add_histogram_value": true,
    "sources": ["mock_source1"]
  })";

  auto def = ParseDefinition(json);
  events_ =
      std::make_unique<TimePeriodEventsIntermediate>(std::move(def), this);
  ASSERT_TRUE(events_->Init());

  ASSERT_TRUE(mock_intermediate1_);

  // Add value on day 1
  mock_intermediate1_->value_ = base::Value(10);
  auto result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 10);

  // Advance 3 days and add another value
  task_environment_.AdvanceClock(base::Days(3));
  mock_intermediate1_->value_ = base::Value(20);
  result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 30);  // Should include both values

  // Advance 5 more days (8 days total) - first value should expire
  task_environment_.AdvanceClock(base::Days(5));
  mock_intermediate1_->value_ = base::Value(5);
  result = events_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 25);  // Only second and third values
}

TEST_F(P3ATimePeriodEventsIntermediateTest, GetStorageKeys) {
  const char json[] = R"({
    "storage_key": "test_key",
    "period_days": 28,
    "sources": ["mock_source1", "mock_source2"]
  })";

  auto def = ParseDefinition(json);
  events_ =
      std::make_unique<TimePeriodEventsIntermediate>(std::move(def), this);
  ASSERT_TRUE(events_->Init());

  auto keys = events_->GetStorageKeys();
  EXPECT_EQ(keys.size(), 3u);
  EXPECT_TRUE(keys.contains(kTestTimePeriodPref));
  EXPECT_TRUE(keys.contains("mock_key1"));
  EXPECT_TRUE(keys.contains("mock_key2"));
}

}  // namespace p3a

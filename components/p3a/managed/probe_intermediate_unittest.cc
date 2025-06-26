/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/probe_intermediate.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

class P3AProbeIntermediateTest : public testing::Test,
                                 public RemoteMetricIntermediate::Delegate {
 public:
  P3AProbeIntermediateTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  // RemoteMetricIntermediate::Delegate implementation
  void TriggerUpdate() override { trigger_update_called_ = true; }
  TimePeriodStorage* GetTimePeriodStorage(std::string_view, int) override {
    return nullptr;
  }
  std::unique_ptr<RemoteMetricIntermediate> GetIntermediateInstance(
      const base::Value&) override {
    return nullptr;
  }

 protected:
  ProbeIntermediateDefinition ParseDefinition(std::string_view json) {
    auto definition_value = base::JSONReader::Read(json);
    EXPECT_TRUE(definition_value.has_value());

    ProbeIntermediateDefinition definition;
    base::JSONValueConverter<ProbeIntermediateDefinition> converter;
    EXPECT_TRUE(converter.Convert(definition_value->GetDict(), &definition));

    return definition;
  }

  base::test::TaskEnvironment task_environment_;
  bool trigger_update_called_ = false;
};

TEST_F(P3AProbeIntermediateTest, InitFailsIfHistogramNameEmpty) {
  // Test with empty histogram name using JSON
  const char json[] = R"({
    "histogram_name": ""
  })";

  auto def = ParseDefinition(json);
  EXPECT_TRUE(def.histogram_name.empty());

  ProbeIntermediate probe(std::move(def), this);
  EXPECT_FALSE(probe.Init());
  EXPECT_FALSE(trigger_update_called_);
}

TEST_F(P3AProbeIntermediateTest, NoFilterCachesAnySample) {
  // Test with valid histogram name but no filter using JSON
  const char json[] = R"({
    "histogram_name": "TestHistogram"
  })";

  auto def = ParseDefinition(json);
  EXPECT_EQ(def.histogram_name, "TestHistogram");
  EXPECT_TRUE(def.filter.empty());

  ProbeIntermediate probe(std::move(def), this);
  ASSERT_TRUE(probe.Init());
  EXPECT_TRUE(probe.GetStorageKeys().empty());
  EXPECT_FALSE(trigger_update_called_);
  EXPECT_TRUE(probe.Process().is_none());

  // No filter: any value should be cached
  probe.OnHistogramSample("", 0, 42);
  EXPECT_TRUE(trigger_update_called_);
  auto result = probe.Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 42);
  // After Process, value is cleared
  EXPECT_TRUE(probe.Process().is_none());
}

TEST_F(P3AProbeIntermediateTest, FilterCachesOnlyMatchingSample) {
  // Test with histogram name and filter using JSON
  const char json[] = R"({
    "histogram_name": "TestHistogram",
    "filter": [7, 42]
  })";

  auto def = ParseDefinition(json);
  EXPECT_EQ(def.histogram_name, "TestHistogram");
  EXPECT_EQ(def.filter.size(), 2u);

  ProbeIntermediate probe(std::move(def), this);
  ASSERT_TRUE(probe.Init());

  // Value in filter
  trigger_update_called_ = false;
  probe.OnHistogramSample("TestHistogram", 0, 42);
  EXPECT_TRUE(trigger_update_called_);
  base::Value result = probe.Process();
  EXPECT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 42);

  // Value not in filter
  trigger_update_called_ = false;
  probe.OnHistogramSample("TestHistogram", 0, 99);
  EXPECT_FALSE(trigger_update_called_);
  EXPECT_TRUE(probe.Process().is_none());
}

}  // namespace p3a

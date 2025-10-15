/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/value_map_intermediate.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

class P3AValueMapIntermediateTest : public testing::Test,
                                    public RemoteMetricIntermediate::Delegate {
 public:
  class MockIntermediate : public RemoteMetricIntermediate {
   public:
    MockIntermediate() : RemoteMetricIntermediate(nullptr) {}
    ~MockIntermediate() override = default;

    bool Init() override { return true; }
    base::Value Process() override { return value_.Clone(); }
    base::flat_set<std::string_view> GetStorageKeys() const override {
      return {{"mock_key"}};
    }
    void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) override {}

    base::Value value_;
  };

  P3AValueMapIntermediateTest() = default;

  // RemoteMetricIntermediate::Delegate implementation
  void TriggerUpdate() override {}
  TimePeriodStorage* GetTimePeriodStorage(std::string_view, int) override {
    return nullptr;
  }
  std::unique_ptr<RemoteMetricIntermediate> GetIntermediateInstance(
      const base::Value& source) override {
    if (source.is_string() && source.GetString() == "mock_source") {
      auto mock_intermediate = std::make_unique<MockIntermediate>();
      mock_intermediate_ = mock_intermediate.get();
      return mock_intermediate;
    }
    return nullptr;
  }

 protected:
  ValueMapIntermediateDefinition ParseDefinition(std::string_view json) {
    auto definition_value = base::JSONReader::Read(json);
    EXPECT_TRUE(definition_value.has_value());

    ValueMapIntermediateDefinition definition;
    base::JSONValueConverter<ValueMapIntermediateDefinition> converter;
    EXPECT_TRUE(converter.Convert(definition_value->GetDict(), &definition));

    return definition;
  }

  std::unique_ptr<ValueMapIntermediate> value_map_;
  raw_ptr<MockIntermediate> mock_intermediate_ = nullptr;
};

TEST_F(P3AValueMapIntermediateTest, InitFailsWithEmptyProperties) {
  const char json1[] = R"({
    "map": {
      "true": 1,
      "false": 0
    }
  })";

  auto def1 = ParseDefinition(json1);
  EXPECT_TRUE(def1.source.is_none());
  EXPECT_FALSE(def1.map.empty());

  value_map_ = std::make_unique<ValueMapIntermediate>(std::move(def1), this);
  EXPECT_FALSE(value_map_->Init());

  const char json2[] = R"({
    "source": "mock_source"
  })";

  auto def2 = ParseDefinition(json2);
  EXPECT_FALSE(def2.source.is_none());
  EXPECT_TRUE(def2.map.empty());

  value_map_ = std::make_unique<ValueMapIntermediate>(std::move(def2), this);
  EXPECT_FALSE(value_map_->Init());
}

TEST_F(P3AValueMapIntermediateTest, ProcessMapsBooleanValues) {
  const char json[] = R"({
    "source": "mock_source",
    "map": {
      "true": 0,
      "false": 1
    }
  })";

  auto def = ParseDefinition(json);
  value_map_ = std::make_unique<ValueMapIntermediate>(std::move(def), this);
  ASSERT_TRUE(value_map_->Init());

  ASSERT_TRUE(mock_intermediate_);

  auto result = value_map_->Process();
  EXPECT_TRUE(result.is_none());

  // Test true maps to 1
  mock_intermediate_->value_ = base::Value(true);
  result = value_map_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 0);

  // Test false maps to 0
  mock_intermediate_->value_ = base::Value(false);
  result = value_map_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 1);
}

TEST_F(P3AValueMapIntermediateTest, ProcessMapsIntegerValues) {
  const char json[] = R"({
    "source": "mock_source",
    "map": {
      "1": "low",
      "2": "medium",
      "3": "high"
    }
  })";

  auto def = ParseDefinition(json);
  value_map_ = std::make_unique<ValueMapIntermediate>(std::move(def), this);
  ASSERT_TRUE(value_map_->Init());

  ASSERT_TRUE(mock_intermediate_);

  mock_intermediate_->value_ = base::Value(1);
  auto result = value_map_->Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), "low");

  mock_intermediate_->value_ = base::Value(2);
  result = value_map_->Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), "medium");

  mock_intermediate_->value_ = base::Value(3);
  result = value_map_->Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), "high");
}

TEST_F(P3AValueMapIntermediateTest, ProcessMapsDoubleValues) {
  const char json[] = R"({
    "source": "mock_source",
    "map": {
      "1.5": "one_and_half",
      "2.7": "two_point_seven"
    }
  })";

  auto def = ParseDefinition(json);
  value_map_ = std::make_unique<ValueMapIntermediate>(std::move(def), this);
  ASSERT_TRUE(value_map_->Init());

  ASSERT_TRUE(mock_intermediate_);

  mock_intermediate_->value_ = base::Value(1.0);
  auto result = value_map_->Process();
  EXPECT_TRUE(result.is_none());

  mock_intermediate_->value_ = base::Value(1.5);
  result = value_map_->Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), "one_and_half");

  mock_intermediate_->value_ = base::Value(2.7);
  result = value_map_->Process();
  ASSERT_TRUE(result.is_string());
  EXPECT_EQ(result.GetString(), "two_point_seven");
}

TEST_F(P3AValueMapIntermediateTest, ProcessMapsStringValues) {
  const char json[] = R"({
    "source": "mock_source",
    "map": {
      "apple": 1,
      "banana": 2,
      "cherry": 3
    }
  })";

  auto def = ParseDefinition(json);
  value_map_ = std::make_unique<ValueMapIntermediate>(std::move(def), this);
  ASSERT_TRUE(value_map_->Init());

  ASSERT_TRUE(mock_intermediate_);

  mock_intermediate_->value_ = base::Value("apple");
  auto result = value_map_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 1);

  mock_intermediate_->value_ = base::Value("banana");
  result = value_map_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 2);

  mock_intermediate_->value_ = base::Value("cherry");
  result = value_map_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 3);

  mock_intermediate_->value_ = base::Value("orange");
  result = value_map_->Process();
  EXPECT_TRUE(result.is_none());
}

TEST_F(P3AValueMapIntermediateTest, GetStorageKeys) {
  const char json[] = R"({
    "source": "mock_source",
    "map": {
      "apple": 1,
      "banana": 2
    }
  })";

  auto def = ParseDefinition(json);
  value_map_ = std::make_unique<ValueMapIntermediate>(std::move(def), this);
  ASSERT_TRUE(value_map_->Init());

  auto keys = value_map_->GetStorageKeys();
  EXPECT_EQ(keys.size(), 1u);
  EXPECT_TRUE(keys.contains("mock_key"));
}

}  // namespace p3a

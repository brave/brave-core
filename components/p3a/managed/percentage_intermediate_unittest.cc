/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/percentage_intermediate.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

class P3APercentageIntermediateTest
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

    std::string storage_key_;
    base::Value value_;
  };

  P3APercentageIntermediateTest() = default;

  // RemoteMetricIntermediate::Delegate implementation
  void TriggerUpdate() override {}
  TimePeriodStorage* GetTimePeriodStorage(std::string_view key,
                                          int period_days) override {
    return nullptr;
  }
  std::unique_ptr<RemoteMetricIntermediate> GetIntermediateInstance(
      const base::Value& source) override {
    if (source.is_string()) {
      const std::string& source_str = source.GetString();
      if (source_str == "numerator_source") {
        auto mock = std::make_unique<MockIntermediate>("numerator_key");
        numerator_mock_ = mock.get();
        return mock;
      }
      if (source_str == "denominator_source") {
        auto mock = std::make_unique<MockIntermediate>("denominator_key");
        denominator_mock_ = mock.get();
        return mock;
      }
    }
    return nullptr;
  }

 protected:
  PercentageIntermediateDefinition ParseDefinition(std::string_view json) {
    auto definition_value = base::JSONReader::Read(json);
    EXPECT_TRUE(definition_value.has_value());

    PercentageIntermediateDefinition definition;
    base::JSONValueConverter<PercentageIntermediateDefinition> converter;
    PercentageIntermediateDefinition::RegisterJSONConverter(&converter);
    EXPECT_TRUE(converter.Convert(definition_value->GetDict(), &definition));

    return definition;
  }

  std::unique_ptr<PercentageIntermediate> percentage_;
  raw_ptr<MockIntermediate> numerator_mock_ = nullptr;
  raw_ptr<MockIntermediate> denominator_mock_ = nullptr;
};

TEST_F(P3APercentageIntermediateTest, InitFailsWithMissingFields) {
  const char json1[] = R"({
    "denominator": "denominator_source"
  })";

  auto def1 = ParseDefinition(json1);
  percentage_ = std::make_unique<PercentageIntermediate>(std::move(def1), this);
  EXPECT_FALSE(percentage_->Init());

  const char json2[] = R"({
    "numerator": "numerator_source"
  })";

  auto def2 = ParseDefinition(json2);
  percentage_ = std::make_unique<PercentageIntermediate>(std::move(def2), this);
  EXPECT_FALSE(percentage_->Init());

  EXPECT_FALSE(numerator_mock_);
  EXPECT_FALSE(denominator_mock_);

  const char json3[] = R"({
    "numerator": "unknown_source",
    "denominator": "denominator_source"
  })";

  auto def3 = ParseDefinition(json3);
  percentage_ = std::make_unique<PercentageIntermediate>(std::move(def3), this);
  EXPECT_FALSE(percentage_->Init());

  denominator_mock_ = nullptr;

  const char json4[] = R"({
    "numerator": "numerator_source",
    "denominator": "unknown_source"
  })";

  auto def4 = ParseDefinition(json4);
  percentage_ = std::make_unique<PercentageIntermediate>(std::move(def4), this);
  EXPECT_FALSE(percentage_->Init());
}

TEST_F(P3APercentageIntermediateTest, ProcessCalculatesBasicPercentage) {
  const char json[] = R"({
    "numerator": "numerator_source",
    "denominator": "denominator_source"
  })";

  auto def = ParseDefinition(json);
  percentage_ = std::make_unique<PercentageIntermediate>(std::move(def), this);
  ASSERT_TRUE(percentage_->Init());

  ASSERT_TRUE(numerator_mock_);
  ASSERT_TRUE(denominator_mock_);

  numerator_mock_->value_ = base::Value(25);
  denominator_mock_->value_ = base::Value(200);

  auto result = percentage_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 12);  // 25/200 * 100 = 12.5%
}

TEST_F(P3APercentageIntermediateTest, ProcessWithMultiplier) {
  const char json[] = R"({
    "numerator": "numerator_source",
    "denominator": "denominator_source",
    "multiplier": 2
  })";

  auto def = ParseDefinition(json);
  percentage_ = std::make_unique<PercentageIntermediate>(std::move(def), this);
  ASSERT_TRUE(percentage_->Init());

  ASSERT_TRUE(numerator_mock_);
  ASSERT_TRUE(denominator_mock_);

  numerator_mock_->value_ = base::Value(25);
  denominator_mock_->value_ = base::Value(100);

  auto result = percentage_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 50);  // (25/100 * 100) * 2 = 50
}

TEST_F(P3APercentageIntermediateTest, ProcessHandlesDivisionByZero) {
  const char json[] = R"({
    "numerator": "numerator_source",
    "denominator": "denominator_source"
  })";

  auto def = ParseDefinition(json);
  percentage_ = std::make_unique<PercentageIntermediate>(std::move(def), this);
  ASSERT_TRUE(percentage_->Init());

  ASSERT_TRUE(numerator_mock_);
  ASSERT_TRUE(denominator_mock_);

  numerator_mock_->value_ = base::Value(25);
  denominator_mock_->value_ = base::Value(0);

  auto result = percentage_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 0);  // Division by zero returns 0%
}

TEST_F(P3APercentageIntermediateTest, ProcessHandlesInvalidValues) {
  const char json[] = R"({
    "numerator": "numerator_source",
    "denominator": "denominator_source"
  })";

  auto def = ParseDefinition(json);
  percentage_ = std::make_unique<PercentageIntermediate>(std::move(def), this);
  ASSERT_TRUE(percentage_->Init());

  ASSERT_TRUE(numerator_mock_);
  ASSERT_TRUE(denominator_mock_);

  numerator_mock_->value_ = base::Value("invalid");
  denominator_mock_->value_ = base::Value(100);

  auto result = percentage_->Process();
  EXPECT_TRUE(result.is_none());

  numerator_mock_->value_ = base::Value(25);
  denominator_mock_->value_ = base::Value("invalid");

  result = percentage_->Process();
  EXPECT_TRUE(result.is_none());
}

TEST_F(P3APercentageIntermediateTest, GetStorageKeys) {
  const char json[] = R"({
    "numerator": "numerator_source",
    "denominator": "denominator_source"
  })";

  auto def = ParseDefinition(json);
  percentage_ = std::make_unique<PercentageIntermediate>(std::move(def), this);
  ASSERT_TRUE(percentage_->Init());

  auto keys = percentage_->GetStorageKeys();
  EXPECT_EQ(keys.size(), 2u);
  EXPECT_TRUE(keys.contains("numerator_key"));
  EXPECT_TRUE(keys.contains("denominator_key"));
}

}  // namespace p3a

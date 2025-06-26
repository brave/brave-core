/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/bucket_intermediate.h"

#include <memory>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

class P3ABucketIntermediateTest : public testing::Test,
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

  P3ABucketIntermediateTest() = default;

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
  BucketIntermediateDefinition ParseDefinition(std::string_view json) {
    auto definition_value = base::JSONReader::Read(json);
    EXPECT_TRUE(definition_value.has_value());

    BucketIntermediateDefinition definition;
    base::JSONValueConverter<BucketIntermediateDefinition> converter;
    EXPECT_TRUE(converter.Convert(definition_value->GetDict(), &definition));

    return definition;
  }

  std::unique_ptr<BucketIntermediate> bucket_;
  raw_ptr<MockIntermediate> mock_intermediate_ = nullptr;
};

TEST_F(P3ABucketIntermediateTest, InitFailsWithEmptyProperties) {
  const char json1[] = R"({
    "buckets": [10, 20, 30]
  })";

  auto def1 = ParseDefinition(json1);
  EXPECT_TRUE(def1.source.is_none());
  EXPECT_FALSE(def1.buckets.empty());

  bucket_ = std::make_unique<BucketIntermediate>(std::move(def1), this);
  EXPECT_FALSE(bucket_->Init());

  const char json2[] = R"({
    "source": "mock_source"
  })";

  auto def2 = ParseDefinition(json2);
  EXPECT_FALSE(def2.source.is_none());
  EXPECT_TRUE(def2.buckets.empty());

  bucket_ = std::make_unique<BucketIntermediate>(std::move(def2), this);
  EXPECT_FALSE(bucket_->Init());

  EXPECT_FALSE(mock_intermediate_);
}

TEST_F(P3ABucketIntermediateTest, ProcessReturnsBucketIndex) {
  const char json[] = R"({
    "source": "mock_source",
    "buckets": [10, 20, 30]
  })";

  auto def = ParseDefinition(json);
  bucket_ = std::make_unique<BucketIntermediate>(std::move(def), this);
  ASSERT_TRUE(bucket_->Init());

  // Test values in different buckets
  mock_intermediate_->value_ =
      base::Value(5);  // Below first bucket -> bucket 0
  auto result = bucket_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 0);

  mock_intermediate_->value_ =
      base::Value(10);  // Equal to first bucket -> bucket 0
  result = bucket_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 0);

  mock_intermediate_->value_ =
      base::Value(15);  // Between first and second bucket -> bucket 1
  result = bucket_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 1);

  mock_intermediate_->value_ =
      base::Value(20);  // Equal to second bucket -> bucket 1
  result = bucket_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 1);

  mock_intermediate_->value_ =
      base::Value(30);  // Equal to third bucket -> bucket 2
  result = bucket_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 2);

  mock_intermediate_->value_ =
      base::Value(40);  // Above all buckets -> bucket 3
  result = bucket_->Process();
  ASSERT_TRUE(result.is_int());
  EXPECT_EQ(result.GetInt(), 3);
}

TEST_F(P3ABucketIntermediateTest, ProcessReturnsEmptyOnInvalidSource) {
  const char json[] = R"({
    "source": "mock_source",
    "buckets": [10, 20, 30]
  })";

  auto def = ParseDefinition(json);
  bucket_ = std::make_unique<BucketIntermediate>(std::move(def), this);
  ASSERT_TRUE(bucket_->Init());

  auto result = bucket_->Process();
  EXPECT_TRUE(result.is_none());

  mock_intermediate_->value_ = base::Value("bad_value");

  result = bucket_->Process();
  EXPECT_TRUE(result.is_none());
}

TEST_F(P3ABucketIntermediateTest, GetStorageKeys) {
  const char json[] = R"({
    "source": "mock_source",
    "buckets": [10, 20, 30]
  })";

  auto def = ParseDefinition(json);
  bucket_ = std::make_unique<BucketIntermediate>(std::move(def), this);
  ASSERT_TRUE(bucket_->Init());

  auto keys = bucket_->GetStorageKeys();
  EXPECT_EQ(keys.size(), 1u);
  EXPECT_TRUE(keys.contains("mock_key"));
}

}  // namespace p3a

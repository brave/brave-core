// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_data.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/query_filter/common/constants.h"
#include "brave/components/query_filter/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace query_filter {

class QueryFilterDataTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitWithFeatures(
        {query_filter::features::kQueryFilterComponent}, {});
  }

  void TearDown() override { instance()->ResetRulesForTesting(); }

  std::vector<QueryFilterRule> GetQueryFilterRules() const {
    return instance()->rules();
  }

 protected:
  QueryFilterData* instance() const { return QueryFilterData::GetInstance(); }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST(QueryFilterDataDisabledTest,
     TestFeatureFlagDisabled_ReturnsEmptyInstance) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(
      query_filter::features::kQueryFilterComponent);
  EXPECT_EQ(QueryFilterData::GetInstance(), nullptr);
}

TEST_F(QueryFilterDataTest, TestFeatureFlagEnabled_ReturnsValidInstance) {
  EXPECT_TRUE(instance() != nullptr);
}

TEST_F(QueryFilterDataTest, TestVersion) {
  base::Version version("1.0.0");
  instance()->UpdateVersion(version);
  EXPECT_EQ(instance()->GetVersion(), "1.0.0");
}

TEST_F(QueryFilterDataTest, TestEmptyJsonReturnsEmpty) {
  EXPECT_FALSE(instance()->PopulateDataFromComponent(""));
  EXPECT_TRUE(GetQueryFilterRules().empty());
}

TEST_F(QueryFilterDataTest, TestInvalidJson_ReturnsEmpty) {
  EXPECT_FALSE(instance()->PopulateDataFromComponent("not json"));
  EXPECT_TRUE(GetQueryFilterRules().empty());
}

TEST_F(QueryFilterDataTest, TestInvalidRootTypeReturnsEmpty) {
  constexpr char kJson[] = R"json({
    "include": ["*://*/*"],
    "exclude": [],
    "params": ["__hsfp", "gclid", "fbclid"]
  })json";

  EXPECT_FALSE(instance()->PopulateDataFromComponent(kJson));
  EXPECT_TRUE(GetQueryFilterRules().empty());
}

TEST_F(QueryFilterDataTest, TestNonDictRuleEntryIsIgnored) {
  constexpr char kJson[] = R"json([
    {
      "include": ["*://a/*"],
      "exclude": [],
      "params": ["x"]
    },
    42,
    {
      "include": ["*://b/*"],
      "exclude": [],
      "params": ["y"]
    }
  ])json";

  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  const std::vector<QueryFilterRule>& rules = instance()->rules();
  EXPECT_EQ(rules.size(), 2u);
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://a/*"));
  EXPECT_TRUE(rules[0].exclude.empty());
  EXPECT_THAT(rules[0].params, testing::ElementsAre("x"));
  EXPECT_THAT(rules[1].include, testing::ElementsAre("*://b/*"));
  EXPECT_TRUE(rules[1].exclude.empty());
  EXPECT_THAT(rules[1].params, testing::ElementsAre("y"));
}

TEST_F(QueryFilterDataTest, TestNonStringListItemsAreIgnored) {
  constexpr char kJson[] = R"json([
  {
    "include": ["*://*/*"],
    "exclude": ["ignored", 99],
    "params": ["gclid", 123, "fbclid", null]
  }
])json";

  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  const std::vector<QueryFilterRule>& rules = instance()->rules();
  EXPECT_EQ(rules.size(), 1u);
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_THAT(rules[0].exclude, testing::ElementsAre("ignored"));
  EXPECT_THAT(rules[0].params, testing::ElementsAre("gclid", "fbclid"));
}

TEST_F(QueryFilterDataTest, TestOnMissingIncludeField_EntryIgnored) {
  constexpr char kJson[] = R"json([
    {
      "exclude": ["example.com"],
      "params": ["x"]
    }
  ])json";
  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  EXPECT_EQ(instance()->rules().size(), 0u);
}

TEST_F(QueryFilterDataTest, TestOnMissingExcludeField_EntryIgnored) {
  constexpr char kJson[] = R"json([
    {
      "include": ["example.com"],
      "params": ["x"]
    }
  ])json";
  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  EXPECT_EQ(instance()->rules().size(), 0u);
}

TEST_F(QueryFilterDataTest, TestOnMissingParamsField_EntryIgnored) {
  constexpr char kJson[] = R"json([
    {
      "include": ["example.com"],
      "exclude": []
    }
  ])json";
  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  EXPECT_EQ(instance()->rules().size(), 0u);
}

TEST_F(QueryFilterDataTest, TestOnNonSupportedFields_EntryIgnored) {
  // Missing "exclude" field. The corresponding dict should be ignored
  constexpr char kJson[] = R"json([
    {
      "include": ["example.com"],
      "exclude": [],
      "params": ["x"],
      "some-nonsense": []
    }
  ])json";
  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  EXPECT_EQ(instance()->rules().size(), 0u);
}

TEST_F(QueryFilterDataTest, TestCheckGeneralRulesPopulation) {
  constexpr char kJson[] = R"json(
[
  {
    "include": ["*://*/*"],
    "exclude": [],
    "params": ["__hsfp", "gclid", "fbclid"]
  },
  {
    "include": [
      "*://*.youtube.com/*",
      "*://youtube.com/*",
      "*://youtu.be/*"
    ],
    "exclude": ["*://excluded.youtube.com/*"],
    "params": ["si"]
  }
]
)json";

  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  const std::vector<QueryFilterRule>& rules = instance()->rules();
  EXPECT_EQ(rules.size(), 2u);
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_TRUE(rules[0].exclude.empty());
  EXPECT_THAT(rules[0].params,
              testing::ElementsAre("__hsfp", "gclid", "fbclid"));
  EXPECT_THAT(rules[1].include,
              testing::ElementsAre("*://*.youtube.com/*", "*://youtube.com/*",
                                   "*://youtu.be/*"));
  EXPECT_THAT(rules[1].exclude,
              testing::ElementsAre("*://excluded.youtube.com/*"));
  EXPECT_THAT(rules[1].params, testing::ElementsAre("si"));
}

}  // namespace query_filter

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

  const std::vector<schema::Rule>& GetQueryFilterRules() const {
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
  instance()->UpdateVersion(base::Version("1.0.0"));
  EXPECT_EQ(instance()->GetVersion(), "1.0.0");

  instance()->UpdateVersion(base::Version("2.0.0"));
  EXPECT_EQ(instance()->GetVersion(), "2.0.0");
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

TEST_F(QueryFilterDataTest, TestOneBadRuleEntryIgnoresThatRule) {
  // Bad rule entry 42. Should be ignored.
  constexpr char kJson[] = R"json([
    {
      "include": ["*://a/*"],
      "exclude": [],
      "params": ["x"]
    },
    42,
    {
      "include": ["*://b/*"],
      "exclude": ["foubah"],
      "params": ["y"]
    }
  ])json";

  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  const auto& rules = GetQueryFilterRules();
  ASSERT_EQ(2UL, rules.size());

  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://a/*"));
  EXPECT_TRUE(rules[0].exclude.empty());
  EXPECT_THAT(rules[0].params, testing::ElementsAre("x"));

  EXPECT_THAT(rules[1].include, testing::ElementsAre("*://b/*"));
  EXPECT_THAT(rules[1].exclude, testing::ElementsAre("foubah"));
  EXPECT_THAT(rules[1].params, testing::ElementsAre("y"));
}

TEST_F(QueryFilterDataTest, TestOneBadRuleEntryItemIgnoresThatEntries) {
  // In 1st entry we have couple of integers which doesn't match the expected
  // schema and so that rule will be ignored. Only the second will be loaded.
  constexpr char kJson[] = R"json([
  {
    "include": ["*://*/*"],
    "exclude": ["foubah", 99],
    "params": ["gclid", 123, "fbclid", null]
  },
  {
    "include": ["*://*/*"],
    "exclude": ["foubah"],
    "params": ["gclid", "fbclid"]
  }
])json";

  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  const auto& rules = GetQueryFilterRules();
  ASSERT_EQ(1UL, rules.size());
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_THAT(rules[0].exclude, testing::ElementsAre("foubah"));
  EXPECT_THAT(rules[0].params, testing::ElementsAre("gclid", "fbclid"));
}

TEST_F(QueryFilterDataTest, TestOnMissingIncludeField_EntryIgnored) {
  constexpr char kJson[] = R"json([
    {
      "exclude": ["example.com"],
      "params": ["x"]
    }
  ])json";

  EXPECT_FALSE(instance()->PopulateDataFromComponent(kJson));
  EXPECT_EQ(0UL, instance()->rules().size());
}

TEST_F(QueryFilterDataTest, TestOnMissingExcludeField_EntryIgnored) {
  constexpr char kJson[] = R"json([
    {
      "include": ["example.com"],
      "params": ["x"]
    }
  ])json";

  EXPECT_FALSE(instance()->PopulateDataFromComponent(kJson));
  EXPECT_EQ(0UL, instance()->rules().size());
}

TEST_F(QueryFilterDataTest, TestOnMissingParamsField_EntryIgnored) {
  constexpr char kJson[] = R"json([
    {
      "include": ["example.com"],
      "exclude": []
    }
  ])json";

  EXPECT_FALSE(instance()->PopulateDataFromComponent(kJson));
  EXPECT_EQ(0UL, instance()->rules().size());
}

// This test ensures we could still add newer attributes to the schema
// without breaking the older Brave clients. However, include, exclude and
// params is written on stone.
TEST_F(QueryFilterDataTest,
       TestOnNonSupportedFields_OnlyNonSupportedEntryItemIsIgnored) {
  constexpr char kJson[] = R"json([
    {
      "include": ["example.com"],
      "exclude": [],
      "params": ["x"],
      "non-supported": []
    }
  ])json";
  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson));
  const auto& rules = GetQueryFilterRules();
  ASSERT_EQ(1UL, rules.size());
  EXPECT_THAT(rules[0].include, testing::ElementsAre("example.com"));
  EXPECT_TRUE(rules[0].exclude.empty());
  EXPECT_THAT(rules[0].params, testing::ElementsAre("x"));
}

TEST_F(QueryFilterDataTest, TestCheckGeneralRulesPopulation_IsCorrect) {
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
  const auto& rules = GetQueryFilterRules();
  ASSERT_EQ(2UL, rules.size());
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

TEST_F(QueryFilterDataTest, TestSequentialRulesUpdate_IsCorrect) {
  constexpr char kJson1[] = R"json(
[
  {
    "include": ["*://*/*"],
    "exclude": [],
    "params": ["__hsfp", "gclid", "fbclid"]
  }
]
)json";

  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson1));
  const auto& rules = GetQueryFilterRules();
  ASSERT_EQ(1UL, rules.size());
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_TRUE(rules[0].exclude.empty());
  EXPECT_THAT(rules[0].params,
              testing::ElementsAre("__hsfp", "gclid", "fbclid"));

  constexpr char kJson2[] = R"json(
  [
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

  EXPECT_TRUE(instance()->PopulateDataFromComponent(kJson2));
  const auto& new_rules = GetQueryFilterRules();
  ASSERT_EQ(1UL, new_rules.size());
  EXPECT_THAT(new_rules[0].include,
              testing::ElementsAre("*://*.youtube.com/*", "*://youtube.com/*",
                                   "*://youtu.be/*"));
  EXPECT_THAT(new_rules[0].exclude,
              testing::ElementsAre("*://excluded.youtube.com/*"));
  EXPECT_THAT(new_rules[0].params, testing::ElementsAre("si"));
}

TEST_F(QueryFilterDataTest,
       TestSequentialRulesUpdate_FirstGood_SecondBad_KeepsFirst) {
  constexpr char kGoodJson[] = R"json(
[
  {
    "include": ["*://*/*"],
    "exclude": [],
    "params": ["__hsfp", "gclid", "fbclid"]
  }
]
)json";

  EXPECT_TRUE(instance()->PopulateDataFromComponent(kGoodJson));
  const auto& rules = GetQueryFilterRules();
  ASSERT_EQ(1UL, rules.size());
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_TRUE(rules[0].exclude.empty());
  EXPECT_THAT(rules[0].params,
              testing::ElementsAre("__hsfp", "gclid", "fbclid"));

  constexpr char kBadJson[] = R"json(
  [
    {
    }
  ]
  )json";

  EXPECT_FALSE(instance()->PopulateDataFromComponent(kBadJson));
  const auto& new_rules = GetQueryFilterRules();
  ASSERT_EQ(1UL, new_rules.size());
  EXPECT_THAT(new_rules[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_TRUE(new_rules[0].exclude.empty());
  EXPECT_THAT(new_rules[0].params,
              testing::ElementsAre("__hsfp", "gclid", "fbclid"));
}

}  // namespace query_filter

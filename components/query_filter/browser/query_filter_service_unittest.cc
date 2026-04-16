// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_service.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/query_filter/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace query_filter {

namespace {

// Shape matches
// https://github.com/brave/adblock-lists/blob/master/brave-lists/query-filter.json
constexpr char kSampleQueryFilterJson[] = R"json(
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
    "exclude": [],
    "params": ["si"]
  }
]
)json";

}  // namespace

class QueryFilterServiceTest : public testing::Test {
 public:
  QueryFilterServiceTest() {
    feature_list_.InitWithFeatures({features::kQueryFilterComponent}, {});
  }

  void SetUp() override {
    ASSERT_NE(service(), nullptr);
    service()->ParseRulesJson("");
  }

 protected:
  QueryFilterService* service() const {
    return QueryFilterService::GetInstance();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(QueryFilterServiceTest, EmptyJsonClearsRules) {
  service()->ParseRulesJson(kSampleQueryFilterJson);
  ASSERT_EQ(service()->rules().size(), 2u);

  service()->ParseRulesJson("");
  EXPECT_TRUE(service()->rules().empty());
}

TEST_F(QueryFilterServiceTest, InvalidJsonProducesNoRules) {
  service()->ParseRulesJson("not json");
  EXPECT_TRUE(service()->rules().empty());
}

TEST_F(QueryFilterServiceTest, RootMustBeList) {
  service()->ParseRulesJson("{}");
  EXPECT_TRUE(service()->rules().empty());
}

TEST_F(QueryFilterServiceTest, ParsesIncludeExcludeParams) {
  service()->ParseRulesJson(kSampleQueryFilterJson);

  const std::vector<QueryFilterRule>& rules = service()->rules();
  ASSERT_EQ(rules.size(), 2u);

  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_TRUE(rules[0].exclude.empty());
  EXPECT_THAT(rules[0].params,
              testing::ElementsAre("__hsfp", "gclid", "fbclid"));

  EXPECT_THAT(rules[1].include,
              testing::ElementsAre("*://*.youtube.com/*", "*://youtube.com/*",
                                   "*://youtu.be/*"));
  EXPECT_TRUE(rules[1].exclude.empty());
  EXPECT_THAT(rules[1].params, testing::ElementsAre("si"));
}

TEST_F(QueryFilterServiceTest, SkipsNonObjectEntries) {
  constexpr char kJson[] = R"json(
[
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
]
)json";

  service()->ParseRulesJson(kJson);

  const std::vector<QueryFilterRule>& rules = service()->rules();
  ASSERT_EQ(rules.size(), 2u);
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://a/*"));
  EXPECT_THAT(rules[0].params, testing::ElementsAre("x"));
  EXPECT_THAT(rules[1].include, testing::ElementsAre("*://b/*"));
  EXPECT_THAT(rules[1].params, testing::ElementsAre("y"));
}

TEST_F(QueryFilterServiceTest, IgnoresNonStringListItems) {
  constexpr char kJson[] = R"json(
[
  {
    "include": ["*://*/*"],
    "exclude": ["ignored", 99],
    "params": ["gclid", 123, "fbclid", null]
  }
]
)json";

  service()->ParseRulesJson(kJson);

  const std::vector<QueryFilterRule>& rules = service()->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_THAT(rules[0].exclude, testing::ElementsAre("ignored"));
  EXPECT_THAT(rules[0].params, testing::ElementsAre("gclid", "fbclid"));
}

}  // namespace query_filter

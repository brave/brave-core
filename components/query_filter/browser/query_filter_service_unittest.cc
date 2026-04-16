// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_service.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/query_filter/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace query_filter {

namespace {

// Sample query filter JSON which would be written to a file during setup
// and then read by the query filter service to prepopulate the default rules.
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

class QueryFilterServiceTest : public testing::TestWithParam<bool> {
 public:
  void SetUp() override {
    if (GetParam()) {
      feature_list_.InitWithFeatures({features::kQueryFilterComponent}, {});
    } else {
      feature_list_.InitWithFeatures({}, {features::kQueryFilterComponent});
    }

    if (!GetParam()) {
      return;
    }

    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(component_install_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::WriteFile(
        component_install_dir_.GetPath().AppendASCII("query-filter.json"),
        kSampleQueryFilterJson));
    service()->OnComponentReady(ComponentInstallDir());
    EXPECT_TRUE(base::test::RunUntil(
        [&]() { return service()->rules().size() == 2u; }));
  }

 protected:
  QueryFilterService* service() const {
    return QueryFilterService::GetInstance();
  }

  base::FilePath ComponentInstallDir() const {
    return component_install_dir_.GetPath();
  }

 private:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::DEFAULT,
      base::test::TaskEnvironment::ThreadPoolExecutionMode::DEFAULT};
  base::test::ScopedFeatureList feature_list_;
  base::ScopedTempDir component_install_dir_;
};

TEST_P(QueryFilterServiceTest, OnComponentReady) {
  if (!GetParam()) {
    EXPECT_EQ(service(), nullptr);
    return;
  }

  ASSERT_EQ(service()->rules().size(), 2u);
  const std::vector<QueryFilterRule>& rules = service()->rules();
  EXPECT_THAT(service()->rules()[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_TRUE(service()->rules()[0].exclude.empty());
  EXPECT_THAT(service()->rules()[0].params,
              testing::ElementsAre("__hsfp", "gclid", "fbclid"));
  EXPECT_THAT(service()->rules()[1].include,
              testing::ElementsAre("*://*.youtube.com/*", "*://youtube.com/*",
                                   "*://youtu.be/*"));
  EXPECT_TRUE(rules[1].exclude.empty());
  EXPECT_THAT(rules[1].params, testing::ElementsAre("si"));
}

INSTANTIATE_TEST_SUITE_P(/** no prefix */,
                         QueryFilterServiceTest,
                         ::testing::Values(false, true),
                         [](const ::testing::TestParamInfo<bool>& info) {
                           return info.param ? "QueryFilterComponentEnabled"
                                             : "QueryFilterComponentDisabled";
                         });

// Parse rules json specific tests below.
class QueryFilterParseRulesJsonTest : public testing::Test {};

TEST_F(QueryFilterParseRulesJsonTest, EmptyJson_ReturnsEmpty) {
  auto rules = ParseRulesJson("");
  EXPECT_TRUE(rules.empty());
}

TEST_F(QueryFilterParseRulesJsonTest, InvalidJson_ReturnsEmpty) {
  auto rules = ParseRulesJson("not json");
  EXPECT_TRUE(rules.empty());
}

TEST_F(QueryFilterParseRulesJsonTest, InvalidRootType_ReturnsEmpty) {
  constexpr char kJson[] = R"json({
    "include": ["*://*/*"],
    "exclude": [],
    "params": ["__hsfp", "gclid", "fbclid"]
  })json";

  auto rules = ParseRulesJson(kJson);
  EXPECT_TRUE(rules.empty());
}

TEST_F(QueryFilterParseRulesJsonTest, NonDictRuleEntryIsIgnored) {
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

  auto rules = ParseRulesJson(kJson);
  EXPECT_EQ(rules.size(), 2u);
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://a/*"));
  EXPECT_TRUE(rules[0].exclude.empty());
  EXPECT_THAT(rules[0].params, testing::ElementsAre("x"));
  EXPECT_THAT(rules[1].include, testing::ElementsAre("*://b/*"));
  EXPECT_TRUE(rules[1].exclude.empty());
  EXPECT_THAT(rules[1].params, testing::ElementsAre("y"));
}

TEST_F(QueryFilterParseRulesJsonTest, NonStringListItemsAreIgnored) {
  constexpr char kJson[] = R"json([
  {
    "include": ["*://*/*"],
    "exclude": ["ignored", 99],
    "params": ["gclid", 123, "fbclid", null]
  }
])json";

  auto rules = ParseRulesJson(kJson);
  EXPECT_EQ(rules.size(), 1u);
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://*/*"));
  EXPECT_THAT(rules[0].exclude, testing::ElementsAre("ignored"));
  EXPECT_THAT(rules[0].params, testing::ElementsAre("gclid", "fbclid"));
}

}  // namespace query_filter

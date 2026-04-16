// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/query_filter_service.h"

#include "base/dcheck_is_on.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
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

class QueryFilterServiceTest : public testing::Test {
 public:
  QueryFilterServiceTest() {
    feature_list_.InitWithFeatures({features::kQueryFilterComponent}, {});
  }

  void SetUp() override {
    ASSERT_NE(service(), nullptr);
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(component_install_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::WriteFile(
        component_install_dir_.GetPath().AppendASCII("query-filter.json"),
        kSampleQueryFilterJson));
    service()->OnComponentReady(ComponentInstallDir());
    task_environment_.RunUntilIdle();
  }

  void TryNewRulesUpdate(const std::string& json) {
    service()->ParseRulesJson(json);
  }

  void AssertCurrentRulesMatchDefaultRules() {
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

TEST_F(QueryFilterServiceTest, LoadsAndParsesQueryFilterFileCorrectly) {
  AssertCurrentRulesMatchDefaultRules();
}

// Parse rules json specific tests below.
TEST_F(QueryFilterServiceTest,
       ParseRulesJsonChecks_EmptyJson_ProducesNoNewRules) {
  TryNewRulesUpdate("");
  AssertCurrentRulesMatchDefaultRules();
}

TEST_F(QueryFilterServiceTest,
       ParseRulesJsonChecks_InvalidJson_ProducesNoNewRules) {
  TryNewRulesUpdate("not json");
  AssertCurrentRulesMatchDefaultRules();
}

TEST_F(QueryFilterServiceTest,
       ParseRulesJsonChecks_InvalidRootNodeInJson_ProducesNoNewRules) {
  TryNewRulesUpdate("{}");
  AssertCurrentRulesMatchDefaultRules();
}

// Non-string entries in include/exclude/params violate the schema; DCHECK
// catches mistakes in debug. Release builds skip non-strings (see .cc).
TEST_F(QueryFilterServiceTest,
       ParseRulesJsonChecks_InvalidJson_NonStringListItemsCrashesInDebug) {
  constexpr char kJson[] = R"json(
[
  {
    "include": ["*://*/*"],
    "exclude": ["ignored", 99],
    "params": ["gclid", 123, "fbclid", null]
  }
]
)json";

  if constexpr (DCHECK_IS_ON()) {
    EXPECT_DEATH_IF_SUPPORTED({ TryNewRulesUpdate(kJson); }, "");
  } else {
    TryNewRulesUpdate(kJson);
    const std::vector<QueryFilterRule>& rules = service()->rules();
    ASSERT_EQ(rules.size(), 1u);
    EXPECT_THAT(rules[0].include, testing::ElementsAre("*://*/*"));
    EXPECT_THAT(rules[0].exclude, testing::ElementsAre("ignored"));
    EXPECT_THAT(rules[0].params, testing::ElementsAre("gclid", "fbclid"));
  }
}

TEST_F(QueryFilterServiceTest,
       ParseRulesJsonChecks_ValidJson_SkipsNonObjectEntries) {
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

  TryNewRulesUpdate(kJson);

  const std::vector<QueryFilterRule>& rules = service()->rules();
  ASSERT_EQ(rules.size(), 2u);
  EXPECT_THAT(rules[0].include, testing::ElementsAre("*://a/*"));
  EXPECT_TRUE(rules[0].exclude.empty());
  EXPECT_THAT(rules[0].params, testing::ElementsAre("x"));
  EXPECT_THAT(rules[1].include, testing::ElementsAre("*://b/*"));
  EXPECT_TRUE(rules[1].exclude.empty());
  EXPECT_THAT(rules[1].params, testing::ElementsAre("y"));
}

}  // namespace query_filter

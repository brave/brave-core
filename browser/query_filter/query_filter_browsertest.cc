// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/query_filter/browser/query_filter_service.h"
#include "brave/components/query_filter/common/features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace query_filter {

namespace {

constexpr char kQueryFilterJsonFile[] = "query-filter.json";

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

class QueryFilterBrowserTest : public InProcessBrowserTest {
 public:
  QueryFilterBrowserTest() {
    feature_list_.InitWithFeatures({features::kQueryFilterComponent}, {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    ASSERT_NE(service(), nullptr);
    ASSERT_TRUE(temp_install_dir_.CreateUniqueTempDir());
  }

  QueryFilterService* service() const {
    return QueryFilterService::GetInstance();
  }

  base::FilePath InstallDirPath() const { return temp_install_dir_.GetPath(); }

  void WriteQueryFilterFile(const std::string& contents) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::WriteFile(
        temp_install_dir_.GetPath().AppendASCII(kQueryFilterJsonFile),
        contents));
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  base::ScopedTempDir temp_install_dir_;
};

IN_PROC_BROWSER_TEST_F(QueryFilterBrowserTest,
                       OnComponentReady_LoadsRulesFromDisk_AndParsesCorrectly) {
  WriteQueryFilterFile(kSampleQueryFilterJson);

  service()->OnComponentReady(InstallDirPath());

  ASSERT_TRUE(
      base::test::RunUntil([&]() { return service()->rules().size() == 2u; }));

  const std::vector<QueryFilterRule>& rules = service()->rules();
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

}  // namespace query_filter

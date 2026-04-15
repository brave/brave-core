/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/files/file_util.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/ad_block_service_browsertest.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kAdBlockTestPage[] = "/blocking.html";

}  // namespace

class AdBlockDATCacheBrowserTest : public AdBlockServiceTest,
                                   public testing::WithParamInterface<bool> {
 public:
  AdBlockDATCacheBrowserTest() {
    if (IsCacheEnabled()) {
      feature_list_.InitAndEnableFeature(
          brave_shields::features::kAdblockDATCache);
    } else {
      feature_list_.InitAndDisableFeature(
          brave_shields::features::kAdblockDATCache);
    }
  }

  bool IsCacheEnabled() const { return GetParam(); }

  // Override to skip InstallDefaultAdBlockComponent — on restart we want
  // the engine to load from the cached DAT, not from a freshly installed
  // component.
  void PreRunTestOnMainThread() override {
    PlatformBrowserTest::PreRunTestOnMainThread();
    WaitForAdBlockServiceThreads();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// PRE_ test: Load rules from persisted provider types so DAT caches are
// written. The browser will restart before the actual test runs.
IN_PROC_BROWSER_TEST_P(AdBlockDATCacheBrowserTest,
                       PRE_DATCacheLoadedOnRestart) {
  InstallDefaultAdBlockComponent();

  // Default engine: component filters (persisted via component updater).
  UpdateAdBlockInstanceWithRules("||component-rule.com^");
  // Additional engine: custom filters (persisted to prefs).
  UpdateCustomAdBlockInstanceWithRules("||custom-rule.com^");

  if (IsCacheEnabled()) {
    // Wait for the DAT cache timestamp to be written (indicates DAT files
    // were serialized and written to disk successfully).
    ASSERT_TRUE(base::test::RunUntil([&]() {
      return !local_state()
                  ->GetString(brave_shields::prefs::kAdBlockDATCacheTimestamp)
                  .empty();
    })) << "Timeout waiting for DAT cache timestamp to be written";

    // Verify DAT files are on disk.
    {
      base::ScopedAllowBlockingForTesting allow_blocking;
      base::FilePath cache_dir =
          profile()->GetPath().AppendASCII("adblock_cache");
      ASSERT_TRUE(base::PathExists(cache_dir.AppendASCII("engine0.dat")))
          << "Default engine DAT file not found";
      ASSERT_TRUE(base::PathExists(cache_dir.AppendASCII("engine1.dat")))
          << "Additional engine DAT file not found";
    }
  }

  // All rules should work within this session.
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  NavigateToURL(tab_url);
  content::WebContents* contents = web_contents();
  EXPECT_EQ(true,
            EvalJs(contents, content::JsReplace(
                                 "setExpectations(0, 1, 0, 0); addImage($1)",
                                 embedded_test_server()
                                     ->GetURL("component-rule.com", "/logo.png")
                                     .spec())));
  EXPECT_EQ(true,
            EvalJs(contents, content::JsReplace(
                                 "setExpectations(0, 2, 0, 0); addImage($1)",
                                 embedded_test_server()
                                     ->GetURL("custom-rule.com", "/logo.png")
                                     .spec())));
}

// After restart: with the cache enabled all rules should be active.
// Without the cache only the pref-persisted custom filter rule survives,
// proving the enabled case loads non-persisted rules from the DAT cache.
IN_PROC_BROWSER_TEST_P(AdBlockDATCacheBrowserTest, DATCacheLoadedOnRestart) {
  if (IsCacheEnabled()) {
    // Verify timestamp persisted from PRE_ test.
    EXPECT_FALSE(
        local_state()
            ->GetString(brave_shields::prefs::kAdBlockDATCacheTimestamp)
            .empty())
        << "DAT cache timestamp not persisted";

    {
      base::ScopedAllowBlockingForTesting allow_blocking;
      base::FilePath cache_dir =
          profile()->GetPath().AppendASCII("adblock_cache");
      EXPECT_TRUE(base::PathExists(cache_dir.AppendASCII("engine0.dat")))
          << "Default engine DAT file not found on restart";
      EXPECT_TRUE(base::PathExists(cache_dir.AppendASCII("engine1.dat")))
          << "Additional engine DAT file not found on restart";
    }

    // Wait for cached DATs to be loaded into the engines.
    auto* service = g_brave_browser_process->ad_block_service();
    ASSERT_TRUE(base::test::RunUntil([service]() {
      return service->IsDATLoadedForTesting(true) &&
             service->IsDATLoadedForTesting(false);
    })) << "Timeout waiting for cached DAT files to load";
  }
  WaitForAdBlockServiceThreads();

  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  NavigateToURL(tab_url);
  content::WebContents* contents = web_contents();

  if (IsCacheEnabled()) {
    // Both rules active via cached DATs.
    EXPECT_EQ(true, EvalJs(contents,
                           content::JsReplace(
                               "setExpectations(0, 1, 0, 0); addImage($1)",
                               embedded_test_server()
                                   ->GetURL("component-rule.com", "/logo.png")
                                   .spec())));
    EXPECT_EQ(true,
              EvalJs(contents, content::JsReplace(
                                   "setExpectations(0, 2, 0, 0); addImage($1)",
                                   embedded_test_server()
                                       ->GetURL("custom-rule.com", "/logo.png")
                                       .spec())));
  } else {
    // Component rule is NOT persisted (temp dir gone) — should not block.
    EXPECT_EQ(true, EvalJs(contents,
                           content::JsReplace(
                               "setExpectations(1, 0, 0, 0); addImage($1)",
                               embedded_test_server()
                                   ->GetURL("component-rule.com", "/logo.png")
                                   .spec())));
    // Custom filter rule IS persisted to prefs — should still block.
    EXPECT_EQ(true,
              EvalJs(contents, content::JsReplace(
                                   "setExpectations(1, 1, 0, 0); addImage($1)",
                                   embedded_test_server()
                                       ->GetURL("custom-rule.com", "/logo.png")
                                       .spec())));
  }
}

// Verify that DAT cache is written after a filter set load.
IN_PROC_BROWSER_TEST_P(AdBlockDATCacheBrowserTest,
                       DATCacheWrittenAfterFilterSetLoad) {
  if (!IsCacheEnabled()) {
    GTEST_SKIP() << "Only relevant with DAT cache enabled";
  }

  InstallDefaultAdBlockComponent();
  UpdateAdBlockInstanceWithRules("||some-rule.com^");

  // Wait for the timestamp to be written.
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !local_state()
                ->GetString(brave_shields::prefs::kAdBlockDATCacheTimestamp)
                .empty();
  })) << "Timeout waiting for DAT cache timestamp";

  // Verify the DAT file exists.
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath cache_dir =
        profile()->GetPath().AppendASCII("adblock_cache");
    EXPECT_TRUE(base::PathExists(cache_dir.AppendASCII("engine0.dat")));
  }

  // The rule should be active.
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  NavigateToURL(tab_url);
  EXPECT_EQ(true, EvalJs(web_contents(),
                         content::JsReplace(
                             "setExpectations(0, 1, 0, 0); addImage($1)",
                             embedded_test_server()
                                 ->GetURL("some-rule.com", "/logo.png")
                                 .spec())));
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdBlockDATCacheBrowserTest,
                         testing::Bool(),
                         [](const auto& info) {
                           return info.param ? "Enabled" : "Disabled";
                         });

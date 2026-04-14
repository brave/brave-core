/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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

  // Override to skip InstallDefaultAdBlockComponent — installing a new
  // component changes the provider set and invalidates the DAT cache key,
  // which is exactly what we're trying to test survives a restart.
  void PreRunTestOnMainThread() override {
    PlatformBrowserTest::PreRunTestOnMainThread();
    WaitForAdBlockServiceThreads();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// PRE_ test: Load rules from persisted provider types so both DAT caches are
// written. The browser will restart before the actual test runs.
IN_PROC_BROWSER_TEST_P(AdBlockDATCacheBrowserTest,
                       PRE_DATCacheLoadedOnRestart) {
  // Install the default component first so UpdateAdBlockInstanceWithRules
  // can find the provider. This persists in prefs across restart.
  InstallDefaultAdBlockComponent();

  // Default engine: component filters (persisted via component updater).
  UpdateAdBlockInstanceWithRules("||component-rule.com^");
  // Additional engine: custom filters (persisted to prefs).
  UpdateCustomAdBlockInstanceWithRules("||custom-rule.com^");

  if (IsCacheEnabled()) {
    // Wait for both cache hashes to be written.
    ASSERT_TRUE(base::test::RunUntil([&]() {
      return !local_state()
                  ->GetString(brave_shields::prefs::kAdBlockDefaultCacheHash)
                  .empty() &&
             !local_state()
                  ->GetString(brave_shields::prefs::kAdBlockAdditionalCacheHash)
                  .empty();
    })) << "Timeout waiting for DAT cache hashes to be written";

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
    // Verify state persisted from PRE_ test.
    LOG(INFO) << "Restart: Default cache hash: "
              << local_state()->GetString(
                     brave_shields::prefs::kAdBlockDefaultCacheHash);
    LOG(INFO) << "Restart: Additional cache hash: "
              << local_state()->GetString(
                     brave_shields::prefs::kAdBlockAdditionalCacheHash);
    EXPECT_FALSE(local_state()
                     ->GetString(brave_shields::prefs::kAdBlockDefaultCacheHash)
                     .empty())
        << "Default cache hash not persisted";
    EXPECT_FALSE(
        local_state()
            ->GetString(brave_shields::prefs::kAdBlockAdditionalCacheHash)
            .empty())
        << "Additional cache hash not persisted";

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
    // IsDATLoadedForTesting is set in NotifyOnDATLoaded so it's safe to
    // poll even if the notification fired before this test body ran.
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

// Verify that updating filter rules changes the cache hash.
IN_PROC_BROWSER_TEST_P(AdBlockDATCacheBrowserTest,
                       DATCacheUpdatedOnRuleChange) {
  if (!IsCacheEnabled()) {
    GTEST_SKIP() << "Only relevant with DAT cache enabled";
  }

  InstallDefaultAdBlockComponent();
  UpdateAdBlockInstanceWithRules("||first-rule.com^");

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !local_state()
                ->GetString(brave_shields::prefs::kAdBlockDefaultCacheHash)
                .empty();
  })) << "Timeout waiting for initial DAT cache hash";

  std::string first_hash =
      local_state()->GetString(brave_shields::prefs::kAdBlockDefaultCacheHash);

  UpdateAdBlockInstanceWithRules("||second-rule.com^");

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return local_state()->GetString(
               brave_shields::prefs::kAdBlockDefaultCacheHash) != first_hash;
  })) << "Timeout waiting for DAT cache hash to update";

  EXPECT_NE(
      local_state()->GetString(brave_shields::prefs::kAdBlockDefaultCacheHash),
      first_hash);

  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  NavigateToURL(tab_url);
  EXPECT_EQ(true, EvalJs(web_contents(),
                         content::JsReplace(
                             "setExpectations(0, 1, 0, 0); addImage($1)",
                             embedded_test_server()
                                 ->GetURL("second-rule.com", "/logo.png")
                                 .spec())));
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdBlockDATCacheBrowserTest,
                         testing::Bool(),
                         [](const auto& info) {
                           return info.param ? "Enabled" : "Disabled";
                         });

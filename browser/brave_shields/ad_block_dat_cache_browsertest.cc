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
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/platform_browser_test.h"
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

  // When the DAT cache feature is enabled, the initial filter set load may
  // be suppressed (engine loaded from cached DAT instead). The base class
  // PreRunTestOnMainThread waits for MakeEngineWithRules which won't fire
  // in that case. Skip it and just wait for the service threads.
  void PreRunTestOnMainThread() override {
    PlatformBrowserTest::PreRunTestOnMainThread();
    component_service_manager()->InitializeGatesForTesting();
    WaitForAdBlockServiceThreads();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// PRE_ test: Add custom filter rules (persisted to prefs) and wait for
// DAT caches to be written. The browser will restart before the actual test.
IN_PROC_BROWSER_TEST_P(AdBlockDATCacheBrowserTest,
                       PRE_DATCacheLoadedOnRestart) {
  // Custom filters are persisted to prefs and go into the additional engine.
  UpdateCustomAdBlockInstanceWithRules("||custom-rule.com^");

  if (IsCacheEnabled()) {
    // Wait for the DAT cache timestamp to be written (indicates DAT files
    // were serialized and written to disk successfully).
    ASSERT_TRUE(base::test::RunUntil([&]() {
      return !local_state()
                  ->GetString(
                      brave_shields::prefs::kAdBlockDefaultDATCacheTimestamp)
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

  WaitForAdBlockServiceThreads();

  // Custom rule should work within this session.
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  NavigateToURL(tab_url);
  EXPECT_EQ(true, EvalJs(web_contents(),
                         content::JsReplace(
                             "setExpectations(0, 1, 0, 0); addImage($1)",
                             embedded_test_server()
                                 ->GetURL("custom-rule.com", "/logo.png")
                                 .spec())));
}

// After restart: with cache enabled, the cached DAT loads and custom rules
// are active without a filter set rebuild. Without cache, custom rules still
// work because they're persisted to prefs.
IN_PROC_BROWSER_TEST_P(AdBlockDATCacheBrowserTest, DATCacheLoadedOnRestart) {
  auto* service = g_brave_browser_process->ad_block_service();

  if (IsCacheEnabled()) {
    // Verify timestamp persisted from PRE_ test.
    EXPECT_FALSE(
        local_state()
            ->GetString(brave_shields::prefs::kAdBlockDefaultDATCacheTimestamp)
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

    // Wait for cached DATs to be loaded into both engines.
    ASSERT_TRUE(base::test::RunUntil([service]() {
      return service->IsDATLoadedForTesting(true) &&
             service->IsDATLoadedForTesting(false);
    })) << "Timeout waiting for cached DAT files to load";

    WaitForAdBlockServiceThreads();

    // Verify no filter set rebuild happened — the cached DATs should have
    // been sufficient.
    EXPECT_FALSE(service->IsFilterListLoadedForTesting(true))
        << "Default engine filter set was rebuilt — suppression failed";
    EXPECT_FALSE(service->IsFilterListLoadedForTesting(false))
        << "Additional engine filter set was rebuilt — suppression failed";
  }

  WaitForAdBlockServiceThreads();

  // Custom rule should be active (from cached DAT when enabled, from prefs
  // when disabled).
  GURL tab_url = embedded_test_server()->GetURL("b.com", kAdBlockTestPage);
  NavigateToURL(tab_url);
  EXPECT_EQ(true, EvalJs(web_contents(),
                         content::JsReplace(
                             "setExpectations(0, 1, 0, 0); addImage($1)",
                             embedded_test_server()
                                 ->GetURL("custom-rule.com", "/logo.png")
                                 .spec())));

  // Install the default component with rules. This adds a new provider
  // which should trigger a filter set rebuild — proving suppression only
  // blocks the initial load, not subsequent ones.
  InstallDefaultAdBlockComponent();
  UpdateAdBlockInstanceWithRules("||component-rule.com^");

  // Filter list should now be loaded for the default engine.
  ASSERT_TRUE(base::test::RunUntil([service]() {
    return service->IsFilterListLoadedForTesting(true);
  })) << "Timeout waiting for filter set to load";

  WaitForAdBlockServiceThreads();

  // Re-navigate to reset page-level counters.
  NavigateToURL(tab_url);

  // Custom rule should still be active.
  EXPECT_EQ(true, EvalJs(web_contents(),
                         content::JsReplace(
                             "setExpectations(0, 1, 0, 0); addImage($1)",
                             embedded_test_server()
                                 ->GetURL("custom-rule.com", "/logo.png")
                                 .spec())));

  // Component rule should also be active now.
  EXPECT_EQ(true, EvalJs(web_contents(),
                         content::JsReplace(
                             "setExpectations(0, 2, 0, 0); addImage($1)",
                             embedded_test_server()
                                 ->GetURL("component-rule.com", "/logo.png")
                                 .spec())));
}

INSTANTIATE_TEST_SUITE_P(All,
                         AdBlockDATCacheBrowserTest,
                         testing::Bool(),
                         [](const auto& info) {
                           return info.param ? "Enabled" : "Disabled";
                         });

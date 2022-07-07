/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <vector>

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ui/views/brave_shields/cookie_list_opt_in_bubble_host.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "url/url_constants.h"

namespace brave_shields {

class CookieListOptInBrowserTest : public InProcessBrowserTest {
 public:
  static inline constexpr char kRegionalAdBlockComponentTestId[] =
      "lfgnenkkneohplacnfabidofpgcdpofm";

  static inline constexpr char kRegionalAdBlockComponentTest64PublicKey[] =
      "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqNcRp37CBZCXe1vbmXyobycPxHyE"
      "KNIgNl6p0XBBxtcZcQOijpY70GjCRzgCL7m1+FBo4MR3FXLiF2aPn/"
      "QsUR8t7+zfw3XzBVos4Ssexkqpd4/"
      "4ciASwTXpbuyFOq4Z5dcgJ1afeT9Zj5bmh4ekLpgJ1NzVwCMhEKk6cmSKIaGVo5EEydtlor2"
      "nkUJrSFuZA6tYZ++"
      "4BOfhhCtzrvXTZjg7mTlB6ca21NL4oLwtqvJMtF8ddoumh619BB5wOqxLzntC/"
      "oWyOxf00V5HDC7e/"
      "DRj9J8jLRFLd4EQUO4Mk+kG3MNy0ph9cqdw6zFR7a2H3LGkl4ejsifM1mUDuJL0cwIDAQAB";

  CookieListOptInBrowserTest() : CookieListOptInBrowserTest(true) {}
  ~CookieListOptInBrowserTest() override = default;

 protected:
  explicit CookieListOptInBrowserTest(bool enable_feature) {
    if (enable_feature) {
      scoped_feature_list_.InitWithFeatures(
          {features::kBraveAdblockCookieListOptIn}, {});
    } else {
      scoped_feature_list_.InitWithFeatures(
          {}, {features::kBraveAdblockCookieListOptIn});
    }
  }

  virtual void SetUpLocalState() { InitializeFilterLists(); }

  void PreRunTestOnMainThread() override {
    CookieListOptInBubbleHost::AllowBubbleInBackgroundForTesting();
    SetUpLocalState();
    InProcessBrowserTest::PreRunTestOnMainThread();
  }

  auto* regional_service_manager() {
    return g_brave_browser_process->ad_block_service()
        ->regional_service_manager();
  }

  content::WebContents* GetBubbleWebContents() {
    auto* host = CookieListOptInBubbleHost::FromBrowser(browser());
    return host ? host->GetBubbleWebContentsForTesting() : nullptr;
  }

  void WaitForSessionRestore() {
    if (SessionRestore::IsRestoring(browser()->profile())) {
      base::RunLoop run_loop;
      auto subscription = SessionRestore::RegisterOnSessionRestoredCallback(
          base::BindLambdaForTesting(
              [&run_loop](Profile*, int) { run_loop.Quit(); }));
      run_loop.Run();
    }
  }

  bool IsCookieListFilterEnabled() {
    return regional_service_manager()->IsFilterListEnabled(kCookieListUuid);
  }

 private:
  void InitializeFilterLists() {
    std::vector regional_catalog = {adblock::FilterList(
        kCookieListUuid,
        "https://secure.fanboy.co.nz/fanboy-cookiemonster_ubo.txt",
        "Easylist-Cookie List - Filter Obtrusive Cookie Notices", {},
        "https://forums.lanik.us/", kRegionalAdBlockComponentTestId,
        kRegionalAdBlockComponentTest64PublicKey,
        "Removes obtrusive cookie law notices")};
    regional_service_manager()->SetRegionalCatalog(regional_catalog);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(CookieListOptInBrowserTest, EnableFromBubble) {
  auto* web_contents = GetBubbleWebContents();
  ASSERT_TRUE(web_contents);

  base::RunLoop run_loop;
  PrefChangeRegistrar pref_observer;
  pref_observer.Init(g_browser_process->local_state());
  pref_observer.Add(prefs::kAdBlockRegionalFilters,
                    base::BindLambdaForTesting([this, &run_loop]() {
                      if (IsCookieListFilterEnabled()) {
                        run_loop.Quit();
                      }
                    }));

  ASSERT_EQ(true, content::EvalJs(web_contents, R"js(
    new Promise((resolve) => {
      setInterval(() => {
        const elem = document.querySelector('.opt-in-action button')
        if (elem) {
          resolve(elem)
        }
      }, 30)
    }).then((elem) => {
      elem.click()
      return true
    })
  )js"));

  run_loop.Run();

  EXPECT_TRUE(IsCookieListFilterEnabled());

  ASSERT_TRUE(
      AddTabAtIndex(1, GURL(url::kAboutBlankURL), ui::PAGE_TRANSITION_TYPED));

  WaitForLoadStop(browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_FALSE(GetBubbleWebContents());
}

IN_PROC_BROWSER_TEST_F(CookieListOptInBrowserTest, PRE_MultipleWindowRestore) {
  EXPECT_TRUE(GetBubbleWebContents());

  // Create another browser window.
  CreateBrowser(browser()->profile());

  // Before closing, reset the "show" pref so that the opt-in will be displayed
  // again.
  g_browser_process->local_state()->SetBoolean(
      prefs::kAdBlockCookieListOptInShown, false);
}

IN_PROC_BROWSER_TEST_F(CookieListOptInBrowserTest, MultipleWindowRestore) {
  WaitForSessionRestore();

  auto* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(browser_list->size(), 2ul);

  // Count the number of windows that are showing the bubble.
  auto bubble_count = std::count_if(
      browser_list->begin(), browser_list->end(), [](Browser* browser) {
        if (auto* host = CookieListOptInBubbleHost::FromBrowser(browser)) {
          if (host->GetBubbleWebContentsForTesting()) {
            return true;
          }
        }
        return false;
      });

  // Only one browser window should be showing the bubble on restore. Note that
  // all browser windows will be inactive in tests. Outside of tests, the bubble
  // will only be displayed on active browsers windows.
  EXPECT_EQ(bubble_count, 1);
}

class CookieListOptInPreEnabledBrowserTest : public CookieListOptInBrowserTest {
 public:
  ~CookieListOptInPreEnabledBrowserTest() override = default;

 protected:
  void SetUpLocalState() override {
    CookieListOptInBrowserTest::SetUpLocalState();
    regional_service_manager()->EnableFilterList(kCookieListUuid, true);
  }
};

IN_PROC_BROWSER_TEST_F(CookieListOptInPreEnabledBrowserTest, AlreadyEnabled) {
  WaitForSessionRestore();
  EXPECT_FALSE(GetBubbleWebContents());
  EXPECT_FALSE(g_browser_process->local_state()->GetBoolean(
      prefs::kAdBlockCookieListOptInShown));
}

class CookieListOptInFirstRunBrowserTest : public CookieListOptInBrowserTest {
 public:
  ~CookieListOptInFirstRunBrowserTest() override = default;

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    // In browser tests, `chrome::IsChromeFirstRun()` will return `false`. Force
    // first-run behavior for this test by using a command line flag.
    command_line->AppendSwitch(switches::kForceFirstRun);
  }
};

IN_PROC_BROWSER_TEST_F(CookieListOptInFirstRunBrowserTest, FirstRun) {
  WaitForSessionRestore();
  EXPECT_FALSE(GetBubbleWebContents());
  EXPECT_FALSE(IsCookieListFilterEnabled());
  EXPECT_FALSE(g_browser_process->local_state()->GetBoolean(
      prefs::kAdBlockCookieListOptInShown));
}

class CookieListOptInFeatureOffBrowserTest : public CookieListOptInBrowserTest {
 public:
  CookieListOptInFeatureOffBrowserTest() : CookieListOptInBrowserTest(false) {}
  ~CookieListOptInFeatureOffBrowserTest() override = default;
};

IN_PROC_BROWSER_TEST_F(CookieListOptInFeatureOffBrowserTest, FeatureOff) {
  WaitForSessionRestore();
  EXPECT_FALSE(GetBubbleWebContents());
  EXPECT_FALSE(IsCookieListFilterEnabled());
  EXPECT_FALSE(g_browser_process->local_state()->GetBoolean(
      prefs::kAdBlockCookieListOptInShown));
}

}  // namespace brave_shields

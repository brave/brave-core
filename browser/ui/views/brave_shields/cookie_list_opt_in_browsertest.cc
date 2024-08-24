/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <vector>

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/perf/brave_perf_switches.h"
#include "brave/browser/ui/views/brave_shields/cookie_list_opt_in_bubble_host.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/startup/launch_mode_recorder.h"
#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "url/url_constants.h"

namespace brave_shields {

namespace {

auto* GetComponentServiceManager() {
  return g_brave_browser_process->ad_block_service()
      ->component_service_manager();
}

bool IsCookieListFilterEnabled() {
  return GetComponentServiceManager()->IsFilterListEnabled(kCookieListUuid);
}

class CookieListFilterEnabledObserver {
 public:
  CookieListFilterEnabledObserver() {
    pref_observer_.Init(g_browser_process->local_state());
    pref_observer_.Add(prefs::kAdBlockRegionalFilters,
                       base::BindLambdaForTesting([this]() {
                         if (IsCookieListFilterEnabled()) {
                           run_loop_.Quit();
                         }
                       }));
  }

  void Wait() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;
  PrefChangeRegistrar pref_observer_;
};

}  // namespace

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
          {features::kBraveAdblockCookieListOptIn},
          {features::kBraveAdblockCookieListDefault});
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

 private:
  void InitializeFilterLists() {
    std::vector filter_list_catalog = {FilterListCatalogEntry(
        kCookieListUuid,
        "https://secure.fanboy.co.nz/fanboy-cookiemonster_ubo.txt",
        "Easylist-Cookie List - Filter Obtrusive Cookie Notices", {},
        "https://forums.lanik.us/", "Removes obtrusive cookie law notices",
        false, false, false, 0, {}, kRegionalAdBlockComponentTestId,
        kRegionalAdBlockComponentTest64PublicKey)};
    GetComponentServiceManager()->SetFilterListCatalog(filter_list_catalog);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(CookieListOptInBrowserTest, EnableFromBubble) {
  auto* web_contents = GetBubbleWebContents();
  ASSERT_TRUE(web_contents);

  CookieListFilterEnabledObserver enabled_observer;

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

  enabled_observer.Wait();

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

IN_PROC_BROWSER_TEST_F(CookieListOptInBrowserTest, FirstRun) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  StartupBrowserCreatorImpl creator(base::FilePath(), command_line,
                                    chrome::startup::IsFirstRun::kYes);

  creator.Launch(browser()->profile(), chrome::startup::IsProcessStartup::kNo,
                 /*restore_tabbed_browser=*/true);

  Browser* new_browser = chrome::FindBrowserWithProfile(browser()->profile());
  ASSERT_TRUE(new_browser);
  TabStripModel* tab_strip = new_browser->tab_strip_model();
  ASSERT_EQ(1, tab_strip->count());
  content::WebContents* web_contents = tab_strip->GetWebContentsAt(0);
  content::TestNavigationObserver observer(web_contents, 1);
  observer.Wait();

  EXPECT_FALSE(CookieListOptInBubbleHost::FromBrowser(new_browser));
}

class CookieListOptInPrefSwitchBrowserTest : public CookieListOptInBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    CookieListOptInBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(
        perf::switches::kEnableBraveFeaturesForPerfTesting);
  }
};

IN_PROC_BROWSER_TEST_F(CookieListOptInPrefSwitchBrowserTest,
                       EnableByPerfSwitch) {
  if (!IsCookieListFilterEnabled()) {
    CookieListFilterEnabledObserver enabled_observer;
    enabled_observer.Wait();
  }

  EXPECT_TRUE(IsCookieListFilterEnabled());
}

class CookieListOptInPreEnabledBrowserTest : public CookieListOptInBrowserTest {
 public:
  ~CookieListOptInPreEnabledBrowserTest() override = default;

 protected:
  void SetUpLocalState() override {
    CookieListOptInBrowserTest::SetUpLocalState();

    GetComponentServiceManager()->EnableFilterList(kCookieListUuid, true);

    // Since `AdBlockRegionalServiceManager::EnableFilterList` modifies local
    // state asynchronously in a posted task, waiting for the update to complete
    // can cause a race condition in which a browser window is displayed before
    // the update has occurred (particularly on macOS). Instead of waiting,
    // update local state directly before proceeding.
    ScopedDictPrefUpdate pref_update(g_browser_process->local_state(),
                                     prefs::kAdBlockRegionalFilters);
    base::Value::Dict entry;
    entry.Set("enabled", true);
    pref_update->Set(kCookieListUuid, std::move(entry));
  }
};

IN_PROC_BROWSER_TEST_F(CookieListOptInPreEnabledBrowserTest, AlreadyEnabled) {
  WaitForSessionRestore();
  EXPECT_FALSE(GetBubbleWebContents());
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
  EXPECT_TRUE(IsCookieListFilterEnabled());
  EXPECT_FALSE(g_browser_process->local_state()->GetBoolean(
      prefs::kAdBlockCookieListOptInShown));
}

}  // namespace brave_shields

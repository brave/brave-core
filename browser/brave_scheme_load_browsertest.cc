/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/pattern.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/omnibox/browser/location_bar_model.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/no_renderer_crashes_assertion.h"
#include "net/dns/mock_host_resolver.h"

class BraveSchemeLoadBrowserTest : public InProcessBrowserTest,
                                   public TabStripModelObserver {
 public:
  void SetUpOnMainThread() override {
    embedded_test_server()->ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    ASSERT_TRUE(embedded_test_server()->Start());
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  PrefService* prefs() {
    return user_prefs::UserPrefs::Get(browser()->profile());
  }

  // TabStripModelObserver overrides:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override {
    if (change.type() == TabStripModelChange::kInserted) {
      quit_closure_.Run();
    }
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(GURL url) {
    ui_test_utils::UrlLoadObserver load_complete(url);
    browser()->OpenURL(
        content::OpenURLParams(url, content::Referrer(),
                               WindowOpenDisposition::CURRENT_TAB,
                               ui::PAGE_TRANSITION_TYPED, false),
        /*navigation_handle_callback=*/{});
    load_complete.Wait();
    EXPECT_EQ(active_contents()->GetLastCommittedURL(), url);
    return true;
  }

  bool NavigateToURLUntilLoadStop(const std::string& origin,
                                  const std::string& path) {
    GURL url = embedded_test_server()->GetURL(origin, path);
    return NavigateToURLUntilLoadStop(url);
  }

  // Check loading |url| in guest window is not allowed for an url.
  void TestURLIsNotLoadedInGuestWindow(const GURL& url) {
    Browser* guest_browser = CreateGuestBrowser();
    TabStripModel* guest_model = guest_browser->tab_strip_model();

    // Check guest window has one blank tab.
    EXPECT_EQ("about:blank",
              guest_model->GetActiveWebContents()->GetVisibleURL().spec());
    EXPECT_EQ(1, guest_model->count());
    EXPECT_EQ("about:blank", active_contents()->GetVisibleURL().spec());
    EXPECT_EQ(1, browser()->tab_strip_model()->count());
    // Unable to navigate expected url.
    EXPECT_FALSE(
        content::NavigateToURL(guest_model->GetActiveWebContents(), url));
    auto* entry = guest_model->GetActiveWebContents()
                      ->GetController()
                      .GetLastCommittedEntry();
    EXPECT_EQ(entry->GetPageType(), content::PageType::PAGE_TYPE_ERROR);
    EXPECT_STREQ("about:blank",
                 base::UTF16ToUTF8(
                     browser()->location_bar_model()->GetFormattedFullURL())
                     .c_str());
    EXPECT_EQ(1, browser()->tab_strip_model()->count());
  }

  // Check loading |url| in private window is redirected to normal
  // window.
  void TestURLIsNotLoadedInPrivateWindow(const std::string& url) {
    Browser* private_browser = CreateIncognitoBrowser(nullptr);
    TabStripModel* private_model = private_browser->tab_strip_model();

    // Check normal & private window have one blank tab.
    EXPECT_EQ("about:blank",
              private_model->GetActiveWebContents()->GetVisibleURL().spec());
    EXPECT_EQ(1, private_model->count());
    EXPECT_EQ("about:blank", active_contents()->GetVisibleURL().spec());
    EXPECT_EQ(1, browser()->tab_strip_model()->count());

    browser()->tab_strip_model()->AddObserver(this);

    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();

    // Load url to private window.
    NavigateParams params(
        private_browser, GURL(url), ui::PAGE_TRANSITION_TYPED);
    Navigate(&params);

    run_loop.Run();

    browser()->tab_strip_model()->RemoveObserver(this);

    EXPECT_STREQ(url.c_str(),
                 base::UTF16ToUTF8(browser()->location_bar_model()
                      ->GetFormattedFullURL()).c_str());
    EXPECT_EQ(2, browser()->tab_strip_model()->count());
    // Private window stays as initial state.
    EXPECT_EQ("about:blank",
              private_model->GetActiveWebContents()->GetVisibleURL().spec());
    EXPECT_EQ(1, private_browser->tab_strip_model()->count());
  }

  void TestURLIsLoadedInPrivateWindow(const std::string& url) {
    Browser* private_browser = CreateIncognitoBrowser();
    TabStripModel* private_model = private_browser->tab_strip_model();
    EXPECT_EQ("about:blank",
              private_model->GetActiveWebContents()->GetVisibleURL().spec());

    content::WebContents* web_contents = private_model->GetActiveWebContents();
    EXPECT_TRUE(content::NavigateToURL(web_contents, GURL(url)));

    content::WaitForLoadStop(web_contents);

    EXPECT_EQ(url, web_contents->GetVisibleURL().spec());
    EXPECT_EQ(web_contents->GetVisibleURL().spec(), url);
  }

  base::RepeatingClosure quit_closure_;
};

// Test whether brave page is not loaded from different host by window.open().
IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest, NotAllowedToLoadTest) {
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("example.com", "/brave_scheme_load.html"));
  content::WebContentsConsoleObserver console_observer(active_contents());
  console_observer.SetPattern(
      "Not allowed to load local resource: brave://settings/");

  ASSERT_TRUE(content::ExecJs(
      active_contents(),
      "window.domAutomationController.send(openBraveSettings())"));
  ASSERT_TRUE(console_observer.Wait());
}

// Test whether brave page is not loaded from different host by window.open().
IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       NotAllowedToLoadTestByWindowOpenWithNoOpener) {
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("example.com", "/brave_scheme_load.html"));
  content::WebContentsConsoleObserver console_observer(active_contents());
  console_observer.SetPattern(
      "Not allowed to load local resource: brave://settings/");

  ASSERT_TRUE(content::ExecJs(
      active_contents(),
      "window.domAutomationController.send(openBraveSettingsWithNoOpener())"));
  ASSERT_TRUE(console_observer.Wait());
}

// Test whether brave page is not loaded from different host directly by
// location.replace().
IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       NotAllowedToDirectReplaceTest) {
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("example.com", "/brave_scheme_load.html"));
  content::WebContentsConsoleObserver console_observer(active_contents());
  console_observer.SetPattern(
      "Not allowed to load local resource: brave://settings/");

  ASSERT_TRUE(content::ExecJs(
      active_contents(),
      "window.domAutomationController.send(replaceToBraveSettingsDirectly())"));
  ASSERT_TRUE(console_observer.Wait());
}

// Test whether brave page is not loaded from different host indirectly by
// location.replace().
IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       NotAllowedToIndirectReplaceTest) {
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("example.com", "/brave_scheme_load.html"));
  auto* initial_active_tab = active_contents();
  content::WebContentsConsoleObserver console_observer(initial_active_tab);
  console_observer.SetPattern(
      "Not allowed to load local resource: brave://settings/");

  ASSERT_TRUE(content::ExecJs(initial_active_tab,
                              "window.domAutomationController.send("
                              "replaceToBraveSettingsIndirectly())"));
  ASSERT_TRUE(console_observer.Wait());
}

// Test whether brave page is not loaded from chrome page.
IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       NotAllowedToBraveFromChrome) {
  NavigateToURLBlockUntilNavigationsComplete(active_contents(),
                                             GURL("chrome://newtab/"), 1);

  content::WebContentsConsoleObserver console_observer(active_contents());
  console_observer.SetPattern(
      "Not allowed to load local resource: brave://settings/");

  ASSERT_TRUE(
      content::ExecJs(active_contents(), "window.open(\"brave://settings\")"));
  ASSERT_TRUE(console_observer.Wait());
}

// Test whether brave page is not loaded by click.
IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest, NotAllowedToBraveByClick) {
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("example.com", "/brave_scheme_load.html"));
  content::WebContentsConsoleObserver console_observer(active_contents());
  console_observer.SetPattern(
      "Not allowed to load local resource: brave://settings/");

  ASSERT_TRUE(content::ExecJs(
      active_contents(),
      "window.domAutomationController.send(gotoBraveSettingsByClick())"));
  ASSERT_TRUE(console_observer.Wait());
}

// Test whether brave page is not loaded by middle click.
IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       NotAllowedToBraveByMiddleClick) {
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("example.com", "/brave_scheme_load.html"));
  content::WebContentsConsoleObserver console_observer(active_contents());
  console_observer.SetPattern(
      "Not allowed to load local resource: brave://settings/");

  ASSERT_TRUE(content::ExecJs(
      active_contents(),
      "window.domAutomationController.send(gotoBraveSettingsByMiddleClick())"));
  ASSERT_TRUE(console_observer.Wait());
}

// Check renderer crash happened by observing related notification.
// Some tests are failing for Windows x86 CI,
// See https://github.com/brave/brave-browser/issues/22767
#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)
#define MAYBE_CrashURLTest DISABLED_CrashURLTest
#else
#define MAYBE_CrashURLTest CrashURLTest
#endif
// NOTE: the actual crash functionality is covered upstream in
// chrome/browser/crash_recovery_browsertest.cc
// This test is for the brave:// scheme. This is a regression test added with:
// https://github.com/brave/brave-core/pull/2229)
IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest, MAYBE_CrashURLTest) {
  content::RenderProcessHostWatcher crash_observer(
      browser()->tab_strip_model()->GetActiveWebContents(),
      content::RenderProcessHostWatcher::WATCH_FOR_PROCESS_EXIT);
  content::ScopedAllowRendererCrashes allow_renderer_crashes(active_contents());
  browser()->OpenURL(
      content::OpenURLParams(GURL("brave://crash/"), content::Referrer(),
                             WindowOpenDisposition::CURRENT_TAB,
                             ui::PAGE_TRANSITION_TYPED, false),
      /*navigation_handle_callback=*/{});
  crash_observer.Wait();
}

// Some webuis are not allowed to load in private window.
// Allowed url list are checked by IsURLAllowedInIncognito().
// So, corresponding brave scheme url should be filtered as chrome scheme.
// Ex, brave://settings should be loaded only in normal window because
// chrome://settings is not allowed. When tyring to loading brave://settings in
// private window, it should be loaded in normal window instead of private
// window.
IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       SettingsPageIsNotAllowedInPrivateWindow) {
  TestURLIsNotLoadedInPrivateWindow("brave://settings");
}

IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       RewardsPageIsNotAllowedInPrivateWindow) {
  // Check webui host with non chrome scheme is allowed to load in private
  // window. chrome scheme is used because brave scheme is already replaced with
  // chrome when IsURLAllowedInIncognito() is called. Verify brave scheme url
  // with TestURLIsNotLoadedInPrivateWindow().
  EXPECT_FALSE(
      IsURLAllowedInIncognito(GURL("chrome://rewards"), browser()->profile()));
  EXPECT_TRUE(
      IsURLAllowedInIncognito(GURL("http://rewards"), browser()->profile()));
  TestURLIsNotLoadedInPrivateWindow("brave://rewards");
}

IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       WalletPageIsNotAllowedInPrivateWindow) {
  EXPECT_FALSE(
      IsURLAllowedInIncognito(GURL("chrome://wallet"), browser()->profile()));
  EXPECT_TRUE(
      IsURLAllowedInIncognito(GURL("http://wallet"), browser()->profile()));
  TestURLIsNotLoadedInPrivateWindow("brave://wallet");
  prefs()->SetBoolean(kBraveWalletPrivateWindowsEnabled, true);
  EXPECT_TRUE(
      IsURLAllowedInIncognito(GURL("brave://wallet"), browser()->profile()));
  TestURLIsLoadedInPrivateWindow("chrome://wallet/crypto/onboarding/welcome");
}

IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       WalletPageIsNotAllowedInGuestWindow) {
  TestURLIsNotLoadedInGuestWindow(GURL("brave://wallet"));
}

IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       BraveSyncPageIsNotAllowedInPrivateWindow) {
  EXPECT_FALSE(
      IsURLAllowedInIncognito(GURL("chrome://sync"), browser()->profile()));
  EXPECT_TRUE(
      IsURLAllowedInIncognito(GURL("http://sync"), browser()->profile()));
  TestURLIsNotLoadedInPrivateWindow("brave://sync");
}

IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       BraveWelcomePageIsNotAllowedInPrivateWindow) {
  EXPECT_FALSE(
      IsURLAllowedInIncognito(GURL("chrome://welcome"), browser()->profile()));
  EXPECT_TRUE(
      IsURLAllowedInIncognito(GURL("http://welcome"), browser()->profile()));
  TestURLIsNotLoadedInPrivateWindow("brave://welcome");
}

IN_PROC_BROWSER_TEST_F(BraveSchemeLoadBrowserTest,
                       BraveWelcomePageIsNotAllowedInGuestWindow) {
  TestURLIsNotLoadedInGuestWindow(GURL("brave://welcome"));
}

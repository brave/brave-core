/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore_test_helper.h"
#include "chrome/browser/sessions/session_service_test_helper.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "net/base/features.h"
#include "url/gurl.h"

namespace ephemeral_storage {

// Covers https://github.com/brave/brave-browser/issues/58345: a tab whose
// domain is already queued for ephemeral storage cleanup (Forgetful Mode)
// must not be reopened at startup, whether via session restore or via the
// "Open a specific page or set of pages" startup URLs.
class EphemeralStorageStartupTabsBrowserTest
    : public EphemeralStorageBrowserTest {
 public:
  EphemeralStorageStartupTabsBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        net::features::kBraveForgetFirstPartyStorage);
  }
  ~EphemeralStorageStartupTabsBrowserTest() override = default;

  void SetUpOnMainThread() override {
    EphemeralStorageBrowserTest::SetUpOnMainThread();
    brave_shields_settings_ = BraveShieldsSettingsServiceFactory::GetForProfile(
        browser()->GetProfile());
  }

  void TearDownOnMainThread() override {
    brave_shields_settings_ = nullptr;
    EphemeralStorageBrowserTest::TearDownOnMainThread();
  }

  // Enables Forgetful Mode for `url`'s domain and immediately closes its only
  // tab, so the domain is queued in the ephemeral storage cleanup pref
  // without waiting for the keep-alive timer.
  void EnableForgetfulModeAndScheduleCleanup(const GURL& url) {
    brave_shields_settings_->SetForgetFirstPartyStorageEnabled(true, url);
    CloseWebContents(LoadURLInNewTab(url));
  }

  // Closes `browser` and opens a new window in the same profile, which
  // triggers session restore. Mirrors
  // chrome/browser/sessions/session_restore_browsertest.cc's
  // QuitBrowserAndRestore helper.
  BrowserWindowInterface* QuitBrowserAndRestore(
      BrowserWindowInterface* browser) {
    Profile* profile = browser->GetProfile();

    auto keep_alive = std::make_unique<ScopedKeepAlive>(
        KeepAliveOrigin::SESSION_RESTORE, KeepAliveRestartOption::DISABLED);
    auto profile_keep_alive = std::make_unique<ScopedProfileKeepAlive>(
        profile, ProfileKeepAliveOrigin::kBrowserWindow);
    CloseBrowserSynchronously(browser);

    ui_test_utils::AllBrowserTabAddedWaiter tab_waiter;
    SessionRestoreTestHelper restore_observer;

    // Ensure the session service factory is started, even if it was
    // explicitly shut down.
    SessionServiceTestHelper session_service_helper(profile);
    session_service_helper.SetForceBrowserNotAliveWithNoWindows(true);

    profile->GetDefaultStoragePartition()
        ->OverrideDeleteStaleSessionCleanupDelayForTesting(base::Minutes(0));

    chrome::NewEmptyWindow(profile);

    BrowserWindowInterface* new_browser =
        tabs::TabInterface::GetFromContents(tab_waiter.Wait())
            ->GetBrowserWindowInterface();
    restore_observer.Wait();

    keep_alive.reset();
    profile_keep_alive.reset();
    return new_browser;
  }

  // Closes `browser` and opens a new window in the same profile via
  // StartupBrowserCreatorImpl, which honors SessionStartupPref::URLS (i.e.
  // "Open a specific page or set of pages"). Only honored when no tabbed
  // browser window remains, hence the close.
  BrowserWindowInterface* CloseBrowserAndOpenWithStartupURLs(
      BrowserWindowInterface* browser) {
    Profile* profile = browser->GetProfile();

    auto keep_alive = std::make_unique<ScopedKeepAlive>(
        KeepAliveOrigin::BROWSER, KeepAliveRestartOption::DISABLED);
    auto profile_keep_alive = std::make_unique<ScopedProfileKeepAlive>(
        profile, ProfileKeepAliveOrigin::kBrowserWindow);
    CloseBrowserSynchronously(browser);

    // IsFirstRun::kNo, so this exercises a normal subsequent launch rather
    // than triggering Chrome's first-run import/promo flows (which can pull
    // in a homepage/URL from another browser installed on the machine and
    // override the startup URLs pref this test is trying to exercise).
    base::CommandLine dummy(base::CommandLine::NO_PROGRAM);
    StartupBrowserCreatorImpl creator(base::FilePath(), dummy,
                                      chrome::startup::IsFirstRun::kNo);
    ui_test_utils::BrowserCreatedObserver browser_created_observer;
    creator.Launch(profile, chrome::startup::IsProcessStartup::kNo,
                   /*restore_tabbed_browser=*/true);
    BrowserWindowInterface* new_browser = browser_created_observer.Wait();

    keep_alive.reset();
    profile_keep_alive.reset();
    return new_browser;
  }

 protected:
  raw_ptr<brave_shields::BraveShieldsSettingsService> brave_shields_settings_ =
      nullptr;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Session restore must not reopen the sole tab of a window when that
// tab's domain is in Forgetful Mode; a fallback blank tab is shown instead.
IN_PROC_BROWSER_TEST_F(EphemeralStorageStartupTabsBrowserTest,
                       SessionRestoreSkipsSoleForgetfulTab) {
  Profile* profile = browser()->GetProfile();
  brave_shields_settings_->SetForgetFirstPartyStorageEnabled(
      true, a_site_ephemeral_storage_url_);
  SessionStartupPref::SetStartupPref(
      profile, SessionStartupPref(SessionStartupPref::LAST));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));

  BrowserWindowInterface* new_browser = QuitBrowserAndRestore(browser());
  ASSERT_TRUE(new_browser);

  TabStripModel* tab_strip = new_browser->GetTabStripModel();
  ASSERT_EQ(1, tab_strip->count());
  EXPECT_EQ(chrome::ChromeUINewTabURLAsGURL(),
            tab_strip->GetWebContentsAt(0)->GetVisibleURL());
}

// Session restore must skip only the tab whose domain is in Forgetful
// Mode, while restoring the other tabs normally.
IN_PROC_BROWSER_TEST_F(EphemeralStorageStartupTabsBrowserTest,
                       SessionRestoreSkipsForgetfulTabAmongSeveral) {
  Profile* profile = browser()->GetProfile();
  brave_shields_settings_->SetForgetFirstPartyStorageEnabled(
      true, a_site_ephemeral_storage_url_);
  SessionStartupPref::SetStartupPref(
      profile, SessionStartupPref(SessionStartupPref::LAST));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));
  ASSERT_TRUE(LoadURLInNewTab(b_site_ephemeral_storage_url_));

  BrowserWindowInterface* new_browser = QuitBrowserAndRestore(browser());
  ASSERT_TRUE(new_browser);

  TabStripModel* tab_strip = new_browser->GetTabStripModel();
  ASSERT_EQ(1, tab_strip->count());
  EXPECT_EQ(b_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(0)->GetVisibleURL());
}

// The "Open a specific page or set of pages" startup URLs must not open
// a tab for a URL whose domain is in Forgetful Mode. When that URL is the
// only startup URL, a single fallback NTP tab is opened instead.
IN_PROC_BROWSER_TEST_F(EphemeralStorageStartupTabsBrowserTest,
                       StartupURLsSkipsSoleForgetfulURL) {
  Profile* profile = browser()->GetProfile();
  EnableForgetfulModeAndScheduleCleanup(a_site_ephemeral_storage_url_);

  SessionStartupPref pref(SessionStartupPref::URLS);
  pref.urls = {a_site_ephemeral_storage_url_};
  SessionStartupPref::SetStartupPref(profile, pref);

  BrowserWindowInterface* new_browser =
      CloseBrowserAndOpenWithStartupURLs(browser());
  ASSERT_TRUE(new_browser);

  TabStripModel* tab_strip = new_browser->GetTabStripModel();
  ASSERT_EQ(1, tab_strip->count());
  EXPECT_EQ(chrome::ChromeUINewTabURLAsGURL(),
            tab_strip->GetWebContentsAt(0)->GetVisibleURL());
}

// The "Open a specific page or set of pages" startup URLs must skip
// only the URL whose domain is in Forgetful Mode, opening the rest normally.
IN_PROC_BROWSER_TEST_F(EphemeralStorageStartupTabsBrowserTest,
                       StartupURLsSkipsForgetfulURLAmongSeveral) {
  Profile* profile = browser()->GetProfile();
  EnableForgetfulModeAndScheduleCleanup(a_site_ephemeral_storage_url_);

  SessionStartupPref pref(SessionStartupPref::URLS);
  pref.urls = {a_site_ephemeral_storage_url_, b_site_ephemeral_storage_url_};
  SessionStartupPref::SetStartupPref(profile, pref);

  BrowserWindowInterface* new_browser =
      CloseBrowserAndOpenWithStartupURLs(browser());
  ASSERT_TRUE(new_browser);

  TabStripModel* tab_strip = new_browser->GetTabStripModel();
  ASSERT_EQ(1, tab_strip->count());
  EXPECT_EQ(b_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(0)->GetVisibleURL());
}

// When the active tab's domain is in Forgetful Mode, session restore must
// skip it while restoring the rest, and since none of the restored tabs were
// marked active in the session data, the first remaining tab becomes active.
IN_PROC_BROWSER_TEST_F(
    EphemeralStorageStartupTabsBrowserTest,
    SessionRestoreActivatesFirstTabWhenActiveForgetfulTabSkipped) {
  Profile* profile = browser()->GetProfile();
  brave_shields_settings_->SetForgetFirstPartyStorageEnabled(
      true, c_site_ephemeral_storage_url_);
  SessionStartupPref::SetStartupPref(
      profile, SessionStartupPref(SessionStartupPref::LAST));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));
  ASSERT_TRUE(LoadURLInNewTab(b_site_ephemeral_storage_url_));
  ASSERT_TRUE(LoadURLInNewTab(c_site_ephemeral_storage_url_));
  ASSERT_EQ(2, browser()->GetTabStripModel()->active_index());

  BrowserWindowInterface* new_browser = QuitBrowserAndRestore(browser());
  ASSERT_TRUE(new_browser);

  TabStripModel* tab_strip = new_browser->GetTabStripModel();
  ASSERT_EQ(2, tab_strip->count());
  EXPECT_EQ(a_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(0)->GetVisibleURL());
  EXPECT_EQ(b_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(1)->GetVisibleURL());
  EXPECT_EQ(0, tab_strip->active_index());
}

// When an inactive tab's domain is in Forgetful Mode, session restore must
// skip only that tab and keep the originally active tab active.
IN_PROC_BROWSER_TEST_F(
    EphemeralStorageStartupTabsBrowserTest,
    SessionRestoreKeepsActiveTabWhenInactiveForgetfulTabSkipped) {
  Profile* profile = browser()->GetProfile();
  brave_shields_settings_->SetForgetFirstPartyStorageEnabled(
      true, c_site_ephemeral_storage_url_);
  SessionStartupPref::SetStartupPref(
      profile, SessionStartupPref(SessionStartupPref::LAST));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));
  ASSERT_TRUE(LoadURLInNewTab(b_site_ephemeral_storage_url_));
  ASSERT_TRUE(LoadURLInNewTab(c_site_ephemeral_storage_url_));
  browser()->GetTabStripModel()->ActivateTabAt(1);
  ASSERT_EQ(1, browser()->GetTabStripModel()->active_index());

  BrowserWindowInterface* new_browser = QuitBrowserAndRestore(browser());
  ASSERT_TRUE(new_browser);

  TabStripModel* tab_strip = new_browser->GetTabStripModel();
  ASSERT_EQ(2, tab_strip->count());
  EXPECT_EQ(a_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(0)->GetVisibleURL());
  EXPECT_EQ(b_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(1)->GetVisibleURL());
  EXPECT_EQ(1, tab_strip->active_index());
}

}  // namespace ephemeral_storage

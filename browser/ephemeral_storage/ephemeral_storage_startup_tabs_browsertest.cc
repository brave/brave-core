/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
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

// Verifies that a tab whose domain is already queued for ephemeral storage
// cleanup (Forgetful Mode) is not reopened at startup.
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
    brave_shields_settings_service_ =
        BraveShieldsSettingsServiceFactory::GetForProfile(
            browser()->GetProfile());
  }

  void TearDownOnMainThread() override {
    brave_shields_settings_service_ = nullptr;
    EphemeralStorageBrowserTest::TearDownOnMainThread();
  }

  // Enables Forgetful Mode for `url`'s domain and queues it for the ephemeral
  // storage cleanup without waiting for the keep-alive timer.
  void EnableForgetfulModeAndScheduleCleanup(const GURL& url) {
    brave_shields_settings_service_->SetForgetFirstPartyStorageEnabled(true,
                                                                       url);
    CloseWebContents(LoadURLInNewTab(url));
  }

  // Closes `browser` and opens a new window in the same profile, which
  // triggers session restore.
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

  // Closes `browser` and opens a new window in the same profile which triggers
  // the predefined URLs opening
  BrowserWindowInterface* CloseBrowserAndOpenWithStartupURLs(
      BrowserWindowInterface* browser) {
    Profile* profile = browser->GetProfile();

    auto keep_alive = std::make_unique<ScopedKeepAlive>(
        KeepAliveOrigin::BROWSER, KeepAliveRestartOption::DISABLED);
    auto profile_keep_alive = std::make_unique<ScopedProfileKeepAlive>(
        profile, ProfileKeepAliveOrigin::kBrowserWindow);
    CloseBrowserSynchronously(browser);

    // IsFirstRun::kNo: normal launch, avoids first-run URL override.
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
  raw_ptr<brave_shields::BraveShieldsSettingsService>
      brave_shields_settings_service_ = nullptr;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test that, after session restore, a fallback blank tab is shown instead of
// reopening the tab whose domain was cleared by Forgetful Mode.
IN_PROC_BROWSER_TEST_F(EphemeralStorageStartupTabsBrowserTest,
                       SessionRestoreSkipsSoleForgetfulTab) {
  Profile* profile = browser()->GetProfile();
  brave_shields_settings_service_->SetForgetFirstPartyStorageEnabled(
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

// Test that, after session restore, was skipped only the tab, whose domain is
// in Forgetful Mode
IN_PROC_BROWSER_TEST_F(EphemeralStorageStartupTabsBrowserTest,
                       SessionRestoreSkipsForgetfulTabAmongSeveral) {
  Profile* profile = browser()->GetProfile();
  brave_shields_settings_service_->SetForgetFirstPartyStorageEnabled(
      true, a_site_ephemeral_storage_url_);
  SessionStartupPref::SetStartupPref(
      profile, SessionStartupPref(SessionStartupPref::LAST));
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), a_site_ephemeral_storage_url_));
  ASSERT_TRUE(LoadURLInNewTab(b_site_ephemeral_storage_url_));
  TabStripModel* tab_strip = browser()->GetTabStripModel();
  EXPECT_EQ(a_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(0)->GetVisibleURL());
  EXPECT_EQ(b_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(1)->GetVisibleURL());

  BrowserWindowInterface* new_browser = QuitBrowserAndRestore(browser());
  ASSERT_TRUE(new_browser);

  tab_strip = new_browser->GetTabStripModel();
  ASSERT_EQ(1, tab_strip->count());
  EXPECT_EQ(b_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(0)->GetVisibleURL());
}

// Test that startup URLs from "Open a specific page or set of pages" whose
// domains are in Forgetful Mode are not opened, and that a single fallback NTP
// tab opens when no other startup URLs are set.
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

// Test that, when startup is set to "Open a specific page or set of pages"
// only the URL whose domain is in Forgetful Mode is skipped, and all the other
// pages open normally.
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

// Test that when the active tab's domain is in Forgetful Mode, session restore
// skips that tab but restores the others, and the first remaining tab becomes
// active.
IN_PROC_BROWSER_TEST_F(
    EphemeralStorageStartupTabsBrowserTest,
    SessionRestoreActivatesFirstTabWhenActiveForgetfulTabSkipped) {
  Profile* profile = browser()->GetProfile();
  brave_shields_settings_service_->SetForgetFirstPartyStorageEnabled(
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

// Test that, during session restore, an inactive tab whose domain is in
// Forgetful Mode is skipped, and the tab that was originally active stays
// active.
IN_PROC_BROWSER_TEST_F(
    EphemeralStorageStartupTabsBrowserTest,
    SessionRestoreKeepsActiveTabWhenInactiveForgetfulTabSkipped) {
  Profile* profile = browser()->GetProfile();
  brave_shields_settings_service_->SetForgetFirstPartyStorageEnabled(
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

// Verify that startup tabs in an Incognito window are always restored, even
// when their domain uses Forgetful Mode and is queued for cleanup in the
// regular profile.
IN_PROC_BROWSER_TEST_F(EphemeralStorageStartupTabsBrowserTest,
                       StartupURLsNotSkippedForOTRProfile) {
  EnableForgetfulModeAndScheduleCleanup(a_site_ephemeral_storage_url_);

  Profile* otr_profile =
      browser()->GetProfile()->GetPrimaryOTRProfile(/*create_if_needed=*/true);

  // SessionStartupPref::URLS is not honored in Incognito, so the startup URL
  // is passed via the command line instead, which is honored regardless of
  // profile type.
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendArg(a_site_ephemeral_storage_url_.spec());
  StartupBrowserCreatorImpl creator(base::FilePath(), command_line,
                                    chrome::startup::IsFirstRun::kNo);
  ui_test_utils::BrowserCreatedObserver browser_created_observer;
  creator.Launch(otr_profile, chrome::startup::IsProcessStartup::kNo,
                 /*restore_tabbed_browser=*/true);
  BrowserWindowInterface* otr_browser = browser_created_observer.Wait();
  ASSERT_TRUE(otr_browser);
  ASSERT_TRUE(otr_browser->GetProfile()->IsOffTheRecord());

  TabStripModel* tab_strip = otr_browser->GetTabStripModel();
  ASSERT_EQ(1, tab_strip->count());
  EXPECT_EQ(a_site_ephemeral_storage_url_,
            tab_strip->GetWebContentsAt(0)->GetVisibleURL());
}

}  // namespace ephemeral_storage

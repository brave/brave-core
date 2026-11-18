/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_clear_browsing_data.h"

#include <memory>
#include <optional>
#include <tuple>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_features.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/profile_destruction_waiter.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/browsing_data_remover_test_util.h"
#include "content/public/test/test_navigation_observer.h"
#include "url/url_constants.h"

using content::BraveClearBrowsingData;
using content::WebContents;

class BraveClearDataOnExitTest
    : public InProcessBrowserTest,
      public BraveClearBrowsingData::OnExitTestingCallback {
 public:
  BraveClearDataOnExitTest() = default;
  BraveClearDataOnExitTest(const BraveClearDataOnExitTest&) = delete;
  BraveClearDataOnExitTest& operator=(const BraveClearDataOnExitTest&) = delete;

  void SetUpOnMainThread() override {
    BraveClearBrowsingData::SetOnExitTestingCallback(this);
  }

  void TearDownOnMainThread() override {
    // Borrowed from browser_browsertest.cc.
    // Cycle the MessageLoop: one for each browser.
    for (unsigned int i = 0; i < browsers_count_; ++i) {
      content::RunAllPendingInMessageLoop();
    }

    // Run the application event loop to completion, which will cycle the
    // native MessagePump on all platforms.
    base::RunLoop run_loop;
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure());
    run_loop.Run();

    // Take care of any remaining message loop work.
    content::RunAllPendingInMessageLoop();

    // At this point, quit should be for real now.
    ASSERT_EQ(0u, GlobalBrowserCollection::GetInstance()->GetSize());
  }

  void TearDownInProcessBrowserTestFixture() override {
    // Verify expected number of calls to remove browsing data.
    EXPECT_EQ(remove_data_call_count_, expected_remove_data_call_count_);

    BraveClearBrowsingData::SetOnExitTestingCallback(nullptr);
  }

  int remove_data_call_count() { return remove_data_call_count_; }

  void SetExepectedRemoveDataCallCount(int count) {
    expected_remove_data_call_count_ = count;
  }

  void SetExpectedRemoveDataRemovalMasks(uint64_t remove_mask,
                                         uint64_t origin_mask) {
    expected_remove_mask_ = remove_mask;
    expected_origin_mask_ = origin_mask;
  }

  void SetClearAll(PrefService* prefService) {
    prefService->SetBoolean(browsing_data::prefs::kDeleteBrowsingHistoryOnExit,
                            true);
    prefService->SetBoolean(browsing_data::prefs::kDeleteDownloadHistoryOnExit,
                            true);
    prefService->SetBoolean(browsing_data::prefs::kDeleteCacheOnExit, true);
    prefService->SetBoolean(browsing_data::prefs::kDeleteCookiesOnExit, true);
    prefService->SetBoolean(browsing_data::prefs::kDeletePasswordsOnExit, true);
    prefService->SetBoolean(browsing_data::prefs::kDeleteFormDataOnExit, true);
    prefService->SetBoolean(browsing_data::prefs::kDeleteHostedAppsDataOnExit,
                            true);
    prefService->SetBoolean(browsing_data::prefs::kDeleteSiteSettingsOnExit,
                            true);
#if BUILDFLAG(ENABLE_AI_CHAT)
    prefService->SetBoolean(browsing_data::prefs::kDeleteBraveLeoHistoryOnExit,
                            true);
#endif
  }

  uint64_t GetRemoveMaskAll() {
    return chrome_browsing_data_remover::DATA_TYPE_HISTORY |
           content::BrowsingDataRemover::DATA_TYPE_DOWNLOADS |
           content::BrowsingDataRemover::DATA_TYPE_CACHE |
           chrome_browsing_data_remover::DATA_TYPE_SITE_DATA |
           chrome_browsing_data_remover::DATA_TYPE_PASSWORDS |
           chrome_browsing_data_remover::DATA_TYPE_FORM_DATA |
#if BUILDFLAG(ENABLE_AI_CHAT)
           chrome_browsing_data_remover::DATA_TYPE_BRAVE_LEO_HISTORY |
#endif
           chrome_browsing_data_remover::DATA_TYPE_CONTENT_SETTINGS;
  }

  uint64_t GetOriginMaskAll() {
    return content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB |
           content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB;
  }

  // BraveClearBrowsingData::OnExitTestingCallback implementation.
  void BeforeClearOnExitRemoveData(content::BrowsingDataRemover* remover,
                                   uint64_t remove_mask,
                                   uint64_t origin_mask) override {
    remove_data_call_count_++;

    if (expected_remove_mask_) {
      EXPECT_EQ(expected_remove_mask_, remove_mask);
    }
    if (expected_origin_mask_) {
      EXPECT_EQ(expected_origin_mask_, origin_mask);
    }
  }

 protected:
  unsigned int browsers_count_ = 1u;
  int remove_data_call_count_ = 0;
  int expected_remove_data_call_count_ = 0;
  std::optional<uint64_t> expected_remove_mask_;
  std::optional<uint64_t> expected_origin_mask_;
};

IN_PROC_BROWSER_TEST_F(BraveClearDataOnExitTest, NoPrefsSet) {
  // No set preferences to clear data.
  SetExepectedRemoveDataCallCount(0);
  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnExitTest, VerifyRemovalMasks) {
  // Set all clear data on exit preferences and corresponding expected remove
  // mask and origin flags.
  SetClearAll(browser()->GetProfile()->GetPrefs());

  // Given those preferences the following removal mask is expected.
  SetExpectedRemoveDataRemovalMasks(GetRemoveMaskAll(), GetOriginMaskAll());

  // Expect a call to clear data.
  SetExepectedRemoveDataCallCount(1);

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

class BraveClearDataOnExitTwoBrowsersTest : public BraveClearDataOnExitTest {
 public:
  BraveClearDataOnExitTwoBrowsersTest() { browsers_count_ = 2u; }

  BraveClearDataOnExitTwoBrowsersTest(
      const BraveClearDataOnExitTwoBrowsersTest&) = delete;
  BraveClearDataOnExitTwoBrowsersTest& operator=(
      const BraveClearDataOnExitTwoBrowsersTest&) = delete;

 protected:
  // Open a new browser window with the provided |profile|.
  BrowserWindowInterface* NewBrowserWindow(Profile* profile) {
    DCHECK(profile);
    ui_test_utils::BrowserCreatedObserver browser_created_observer;
    chrome::NewEmptyWindow(profile);
    BrowserWindowInterface* browser = browser_created_observer.Wait();
    DCHECK(browser);
    content::WaitForLoadStopWithoutSuccessCheck(
        browser->tab_strip_model()->GetActiveWebContents());
    return browser;
  }

  // Open a new browser window with a guest session.
  BrowserWindowInterface* NewGuestBrowserWindow() {
    ui_test_utils::BrowserCreatedObserver browser_created_observer;
    profiles::SwitchToGuestProfile(base::DoNothing());
    BrowserWindowInterface* browser = browser_created_observer.Wait();
    DCHECK(browser);
    // When a guest |browser| closes a BrowsingDataRemover will be created and
    // executed. It needs a loaded TemplateUrlService or else it hangs on to a
    // CallbackList::Subscription forever.
    Profile* guest = g_browser_process->profile_manager()->GetProfileByPath(
        ProfileManager::GetGuestProfilePath());
    DCHECK(guest);
    search_test_utils::WaitForTemplateURLServiceToLoad(
        TemplateURLServiceFactory::GetForProfile(guest));
    // Navigate to about:blank.
    EXPECT_TRUE(
        ui_test_utils::NavigateToURL(browser, GURL(url::kAboutBlankURL)));
    return browser;
  }

  // Open a new browser window with a new profile.
  BrowserWindowInterface* NewProfileBrowserWindow() {
    base::FilePath path;
    base::PathService::Get(chrome::DIR_USER_DATA, &path);
    path = path.AppendASCII("Profile 2");
    base::ScopedAllowBlockingForTesting allow_blocking;
    // Clean up profile directory when the test is done.
    std::ignore = profile2_dir_.Set(path);
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    size_t starting_number_of_profiles = profile_manager->GetNumberOfProfiles();
    CHECK(base::PathExists(path) || base::CreateDirectory(path))
        << path.MaybeAsASCII();
    Profile* profile = profile_manager->GetProfile(path);
    DCHECK(profile);
    EXPECT_EQ(starting_number_of_profiles + 1,
              profile_manager->GetNumberOfProfiles());

    return NewBrowserWindow(profile);
  }

  // Close the provided |browser| window and wait until done.
  void CloseBrowserWindow(BrowserWindowInterface* browser) {
    ui_test_utils::BrowserDestroyedObserver browser_destroyed_observer(browser);
    chrome::ExecuteCommand(browser, IDC_CLOSE_WINDOW);
    browser_destroyed_observer.Wait();
  }

  // Enable deletion of browsing history on exit.
  void SetDeleteBrowsingHistoryOnExit(Profile* profile) {
    profile->GetPrefs()->SetBoolean(
        browsing_data::prefs::kDeleteBrowsingHistoryOnExit, true);
  }

  void SetDeleteBrowsingHistoryOnExit() {
    SetDeleteBrowsingHistoryOnExit(browser()->GetProfile());
  }

 private:
  base::ScopedTempDir profile2_dir_;
};

IN_PROC_BROWSER_TEST_F(BraveClearDataOnExitTwoBrowsersTest, SameProfile) {
  // Delete browsing history on exit.
  SetDeleteBrowsingHistoryOnExit();
  // Same profile, so expect a single call.
  SetExepectedRemoveDataCallCount(1);

  // Open a second browser window.
  BrowserWindowInterface* second_window =
      NewBrowserWindow(browser()->GetProfile());
  // Close second browser window
  CloseBrowserWindow(second_window);
  EXPECT_EQ(0, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnExitTwoBrowsersTest, OneOTR) {
  // Delete browsing history on exit.
  SetDeleteBrowsingHistoryOnExit();
  // OTR sessions don't count, so expect a single call.
  SetExepectedRemoveDataCallCount(1);

  // Open a second browser window with OTR profile.
  BrowserWindowInterface* second_window = NewBrowserWindow(
      browser()->GetProfile()->GetPrimaryOTRProfile(/*create_if_needed=*/true));
  // Close second browser window
  CloseBrowserWindow(second_window);
  EXPECT_EQ(0, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnExitTwoBrowsersTest, OneOTRExitsLast) {
  // Delete browsing history on exit.
  SetDeleteBrowsingHistoryOnExit();
  // OTR sessions don't count, so expect a single call.
  SetExepectedRemoveDataCallCount(1);

  // Open a second browser window with OTR profile.
  BrowserWindowInterface* second_window = NewBrowserWindow(
      browser()->GetProfile()->GetPrimaryOTRProfile(/*create_if_needed=*/true));

  // Close regular profile window.
  CloseBrowserWindow(browser());
  EXPECT_EQ(0, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(second_window, IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnExitTwoBrowsersTest, OneGuest) {
  // Delete browsing history on exit.
  SetDeleteBrowsingHistoryOnExit();
  // Guest sessions don't count, so expect a single call.
  SetExepectedRemoveDataCallCount(1);

  // Open a second browser window with Guest session.
  BrowserWindowInterface* guest_window = NewGuestBrowserWindow();

  // Close Guest session window: regular profile cleanup shouldn't happen.
  CloseBrowserWindow(guest_window);
  EXPECT_EQ(0, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnExitTwoBrowsersTest, OneGuestExitsLast) {
  // Delete browsing history on exit.
  SetDeleteBrowsingHistoryOnExit();
  // Guest sessions don't count, so expect a single call.
  SetExepectedRemoveDataCallCount(1);

  // Open a second browser window with Guest session.
  BrowserWindowInterface* guest_window = NewGuestBrowserWindow();

  // Close regular profile window.
  CloseBrowserWindow(browser());
  EXPECT_EQ(0, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(guest_window, IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnExitTwoBrowsersTest, TwoProfiles) {
  // Delete browsing history on exit.
  SetDeleteBrowsingHistoryOnExit();

  // Open a second browser window with a different profile.
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  // Delete browsing history for this profile on exit too.
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // Both profiles have browsing data removal set, so expect two calls.
  SetExepectedRemoveDataCallCount(2);

  // Close second profile window.
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(0, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

// With profile unloading turned on, a profile that clears data on exit is
// cleared when its last window closes, then unloads unless something else
// still uses it.
class BraveClearDataOnProfileUnloadTest
    : public BraveClearDataOnExitTwoBrowsersTest {
 public:
  BraveClearDataOnProfileUnloadTest() {
    // Must be set before the browser starts, when clearing on close starts.
    feature_list_.InitAndEnableFeature(features::kDestroyProfileOnBrowserClose);
  }

 protected:
  // Waits until |profile| has no removal left, then lets the clearing reply.
  void WaitForClearingToFinish(Profile* profile) {
    content::BrowsingDataRemover* remover = profile->GetBrowsingDataRemover();
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return remover->GetPendingTaskCountForTesting() == 0; }));
    base::RunLoop().RunUntilIdle();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       ClearsAndUnloadsWhenLastWindowCloses) {
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // Only the second profile clears data on exit.
  SetExepectedRemoveDataCallCount(1);

  ProfileDestructionWaiter second_profile_destruction_waiter(second_profile);
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(1, remove_data_call_count());
  second_profile_destruction_waiter.Wait();

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest, ClearsOnceOnQuit) {
  SetClearAll(browser()->GetProfile()->GetPrefs());
  SetExpectedRemoveDataRemovalMasks(GetRemoveMaskAll(), GetOriginMaskAll());

  // The profile is cleared as its last window closes during the quit. Exit
  // must not clear it a second time.
  SetExepectedRemoveDataCallCount(1);

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       ProfileStillInUseIsClearedAgainOnExit) {
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // Once when its last window closes, and again on exit, because something
  // still used the profile after the first clear.
  SetExepectedRemoveDataCallCount(2);

  auto download_keep_alive = std::make_unique<ScopedProfileKeepAlive>(
      second_profile, ProfileKeepAliveOrigin::kDownloadInProgress);
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(1, remove_data_call_count());
  WaitForClearingToFinish(second_profile);

  // The profile stays held after the download ends, so exit reaches it.
  download_keep_alive.reset();

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       ReopenedProfileIsClearedAgain) {
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);
  const base::FilePath second_profile_path = second_profile->GetPath();

  // Once when its last window closes, and once more when the application
  // quits after the profile was reopened.
  SetExepectedRemoveDataCallCount(2);

  ProfileDestructionWaiter second_profile_destruction_waiter(second_profile);
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(1, remove_data_call_count());
  second_profile_destruction_waiter.Wait();

  base::ScopedAllowBlockingForTesting allow_blocking;
  Profile* reopened_profile =
      g_browser_process->profile_manager()->GetProfile(second_profile_path);
  ASSERT_TRUE(reopened_profile);
  NewBrowserWindow(reopened_profile);

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       ClosedProfileUnloadsWithoutClearing) {
  // Nothing is set to clear on exit, so no calls are expected.
  SetExepectedRemoveDataCallCount(0);

  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();

  ProfileDestructionWaiter second_profile_destruction_waiter(second_profile);
  CloseBrowserWindow(second_profile_window);
  second_profile_destruction_waiter.Wait();

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       ClearsAgainWhenUsedDuringClearing) {
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // Once when its last window closes, and again because a download ran while
  // that clear was in progress.
  SetExepectedRemoveDataCallCount(2);

  content::BrowsingDataRemoverCompletionInhibitor inhibitor(
      second_profile->GetBrowsingDataRemover());
  ProfileDestructionWaiter second_profile_destruction_waiter(second_profile);
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(1, remove_data_call_count());

  // Hold the first clear just before it finishes, and run a download that
  // starts and ends meanwhile.
  inhibitor.BlockUntilNearCompletion();
  {
    ScopedProfileKeepAlive download_keep_alive(
        second_profile, ProfileKeepAliveOrigin::kDownloadInProgress);
  }
  // Let this clear and any later one finish normally.
  inhibitor.Reset();
  inhibitor.ContinueToCompletion();

  // The profile is cleared a second time, then unloads.
  second_profile_destruction_waiter.Wait();
  EXPECT_EQ(2, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       ClearsAgainWhenRunningWorkEndsDuringClearing) {
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // Once when its last window closes, and again because a download that was
  // already running ended during that clear.
  SetExepectedRemoveDataCallCount(2);

  auto download_keep_alive = std::make_unique<ScopedProfileKeepAlive>(
      second_profile, ProfileKeepAliveOrigin::kDownloadInProgress);
  content::BrowsingDataRemoverCompletionInhibitor inhibitor(
      second_profile->GetBrowsingDataRemover());
  ProfileDestructionWaiter second_profile_destruction_waiter(second_profile);
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(1, remove_data_call_count());

  // The download ends just before the first clear finishes.
  inhibitor.BlockUntilNearCompletion();
  download_keep_alive.reset();
  inhibitor.Reset();
  inhibitor.ContinueToCompletion();

  // The profile is cleared a second time, then unloads.
  second_profile_destruction_waiter.Wait();
  EXPECT_EQ(2, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       SettingsTurnedOffDuringClearingStillUnloads) {
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // Only the first close finds anything to clear.
  SetExepectedRemoveDataCallCount(1);

  content::BrowsingDataRemoverCompletionInhibitor inhibitor(
      second_profile->GetBrowsingDataRemover());
  ProfileDestructionWaiter second_profile_destruction_waiter(second_profile);
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(1, remove_data_call_count());

  // While that clear runs, a window opens, the setting is turned off, and the
  // window closes again. The running clear must keep the profile loaded.
  inhibitor.BlockUntilNearCompletion();
  BrowserWindowInterface* reopened_window = NewBrowserWindow(second_profile);
  second_profile->GetPrefs()->SetBoolean(
      browsing_data::prefs::kDeleteBrowsingHistoryOnExit, false);
  CloseBrowserWindow(reopened_window);
  EXPECT_EQ(1, remove_data_call_count());
  inhibitor.Reset();
  inhibitor.ContinueToCompletion();

  // Once the clear finishes, nothing holds the profile, so it unloads.
  second_profile_destruction_waiter.Wait();

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       PrivateWindowDefersClearingUntilItCloses) {
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // Cleared once, when the profile's last window, the private one, closes.
  SetExepectedRemoveDataCallCount(1);

  BrowserWindowInterface* private_window = NewBrowserWindow(
      second_profile->GetPrimaryOTRProfile(/*create_if_needed=*/true));
  ProfileDestructionWaiter second_profile_destruction_waiter(second_profile);

  // The private window still shows the profile, so nothing is cleared yet.
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(0, remove_data_call_count());

  CloseBrowserWindow(private_window);
  EXPECT_EQ(1, remove_data_call_count());
  second_profile_destruction_waiter.Wait();

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       ClearsAgainAfterClosedWindowTearsDown) {
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // Once when the window closes, and again once its teardown has finished.
  SetExepectedRemoveDataCallCount(2);

  // Stand in for a closed window that still holds the profile while it
  // tears down after the first clear has finished.
  auto closing_window_keep_alive = std::make_unique<ScopedProfileKeepAlive>(
      second_profile, ProfileKeepAliveOrigin::kBrowserWindow);
  ProfileDestructionWaiter second_profile_destruction_waiter(second_profile);
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(1, remove_data_call_count());
  WaitForClearingToFinish(second_profile);
  EXPECT_EQ(1, remove_data_call_count());

  // The teardown ends. A later check clears again, then lets it unload.
  closing_window_keep_alive.reset();
  second_profile_destruction_waiter.Wait();
  EXPECT_EQ(2, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

IN_PROC_BROWSER_TEST_F(BraveClearDataOnProfileUnloadTest,
                       RepeatClearsStopAtTheLimitAndExitClears) {
  BrowserWindowInterface* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  Profile* second_profile = second_profile_window->GetProfile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // The close-time clear and two repeats, then the clear at exit.
  SetExepectedRemoveDataCallCount(4);

  content::BrowsingDataRemoverCompletionInhibitor inhibitor(
      second_profile->GetBrowsingDataRemover());
  CloseBrowserWindow(second_profile_window);

  // A download starts and ends during each of three clears in a row.
  for (int clear = 1; clear <= 3; ++clear) {
    inhibitor.BlockUntilNearCompletion();
    EXPECT_EQ(clear, remove_data_call_count());
    {
      ScopedProfileKeepAlive download_keep_alive(
          second_profile, ProfileKeepAliveOrigin::kDownloadInProgress);
    }
    if (clear == 3) {
      inhibitor.Reset();
    }
    inhibitor.ContinueToCompletion();
  }
  base::RunLoop().RunUntilIdle();

  // No fourth repeat. The profile stays held, so exit can clear it.
  EXPECT_EQ(3, remove_data_call_count());
  EXPECT_TRUE(g_browser_process->profile_manager()->GetProfileByPath(
      second_profile->GetPath()));

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

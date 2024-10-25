/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_clear_browsing_data.h"

#include <optional>
#include <tuple>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
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
#include "content/public/test/test_navigation_observer.h"
#include "url/url_constants.h"

using content::BraveClearBrowsingData;
using content::WebContents;

namespace {

class BrowserChangeObserver : public BrowserListObserver {
 public:
  enum class ChangeType {
    kAdded,
    kRemoved,
  };

  BrowserChangeObserver(Browser* browser, ChangeType type)
      : browser_(browser), type_(type) {
    BrowserList::AddObserver(this);
  }

  BrowserChangeObserver(const BrowserChangeObserver&) = delete;
  BrowserChangeObserver& operator=(const BrowserChangeObserver&) = delete;

  ~BrowserChangeObserver() override { BrowserList::RemoveObserver(this); }

  Browser* Wait() {
    run_loop_.Run();
    return browser_;
  }

  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override {
    if (type_ == ChangeType::kAdded) {
      browser_ = browser;
      run_loop_.Quit();
    }
  }

  void OnBrowserRemoved(Browser* browser) override {
    if (browser_ && browser_ != browser)
      return;

    if (type_ == ChangeType::kRemoved) {
      browser_ = browser;
      run_loop_.Quit();
    }
  }

 private:
  raw_ptr<Browser, DanglingUntriaged> browser_ = nullptr;
  ChangeType type_;
  base::RunLoop run_loop_;
};

}  // namespace

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
    for (unsigned int i = 0; i < browsers_count_; ++i)
      content::RunAllPendingInMessageLoop();

    // Run the application event loop to completion, which will cycle the
    // native MessagePump on all platforms.
    base::RunLoop run_loop;
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure());
    run_loop.Run();

    // Take care of any remaining message loop work.
    content::RunAllPendingInMessageLoop();
  }

  void TearDownInProcessBrowserTestFixture() override {
    // Verify expected number of calls to remove browsing data.
    EXPECT_EQ(remove_data_call_count_, expected_remove_data_call_count_);

    // At this point, quit should be for real now.
    ASSERT_EQ(0u, chrome::GetTotalBrowserCount());

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
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
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
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
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

    if (expected_remove_mask_)
      EXPECT_EQ(expected_remove_mask_, remove_mask);
    if (expected_origin_mask_)
      EXPECT_EQ(expected_origin_mask_, origin_mask);
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
  SetClearAll(browser()->profile()->GetPrefs());

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
  Browser* NewBrowserWindow(Profile* profile) {
    DCHECK(profile);
    BrowserChangeObserver bco(nullptr,
                              BrowserChangeObserver::ChangeType::kAdded);
    chrome::NewEmptyWindow(profile);
    Browser* browser = bco.Wait();
    DCHECK(browser);
    content::WaitForLoadStopWithoutSuccessCheck(
        browser->tab_strip_model()->GetActiveWebContents());
    return browser;
  }

  // Open a new browser window with a guest session.
  Browser* NewGuestBrowserWindow() {
    BrowserChangeObserver bco(nullptr,
                              BrowserChangeObserver::ChangeType::kAdded);
    profiles::SwitchToGuestProfile(base::DoNothing());
    Browser* browser = bco.Wait();
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
  Browser* NewProfileBrowserWindow() {
    base::FilePath path;
    base::PathService::Get(chrome::DIR_USER_DATA, &path);
    path = path.AppendASCII("Profile 2");
    base::ScopedAllowBlockingForTesting allow_blocking;
    // Clean up profile directory when the test is done.
    std::ignore = profile2_dir_.Set(path);
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    size_t starting_number_of_profiles = profile_manager->GetNumberOfProfiles();
    if (!base::PathExists(path) && !base::CreateDirectory(path))
      NOTREACHED_IN_MIGRATION()
          << "Could not create directory at " << path.MaybeAsASCII();
    Profile* profile = profile_manager->GetProfile(path);
    DCHECK(profile);
    EXPECT_EQ(starting_number_of_profiles + 1,
              profile_manager->GetNumberOfProfiles());

    return NewBrowserWindow(profile);
  }

  // Close the provided |browser| window and wait until done.
  void CloseBrowserWindow(Browser* browser) {
    BrowserChangeObserver bco(browser,
                              BrowserChangeObserver::ChangeType::kRemoved);
    chrome::ExecuteCommand(browser, IDC_CLOSE_WINDOW);
    EXPECT_EQ(bco.Wait(), browser);
  }

  // Enable deletion of browsing history on exit.
  void SetDeleteBrowsingHistoryOnExit(Profile* profile) {
    profile->GetPrefs()->SetBoolean(
        browsing_data::prefs::kDeleteBrowsingHistoryOnExit, true);
  }

  void SetDeleteBrowsingHistoryOnExit() {
    SetDeleteBrowsingHistoryOnExit(browser()->profile());
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
  Browser* second_window = NewBrowserWindow(browser()->profile());
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
  Browser* second_window = NewBrowserWindow(
      browser()->profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true));
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
  Browser* second_window = NewBrowserWindow(
      browser()->profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true));

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
  Browser* guest_window = NewGuestBrowserWindow();

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
  Browser* guest_window = NewGuestBrowserWindow();

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
  Browser* second_profile_window = NewProfileBrowserWindow();
  DCHECK(second_profile_window);
  // Delete browsing history for this profile on exit too.
  Profile* second_profile = second_profile_window->profile();
  SetDeleteBrowsingHistoryOnExit(second_profile);

  // Both profiles have browsing data removal set, so expect two calls.
  SetExepectedRemoveDataCallCount(2);

  // Close second profile window.
  CloseBrowserWindow(second_profile_window);
  EXPECT_EQ(0, remove_data_call_count());

  // Tell the application to quit.
  chrome::ExecuteCommand(browser(), IDC_EXIT);
}

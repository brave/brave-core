// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "ui/views/view.h"

// An observer that returns back to test code after a new profile is
// initialized.
void OnUnblockOnProfileCreation(base::RunLoop* run_loop,
                                Profile* profile,
                                Profile::CreateStatus status) {
  if (status == Profile::CREATE_STATUS_INITIALIZED)
    run_loop->Quit();
}

class BraveToolbarViewTest : public InProcessBrowserTest {
 public:
  BraveToolbarViewTest() = default;
  ~BraveToolbarViewTest() override = default;

  void SetUpOnMainThread() override {
    Init(browser());
  }

  void Init(Browser* browser) {
    BrowserView* browser_view =
      BrowserView::GetBrowserViewForBrowser(browser);
    ASSERT_NE(browser_view, nullptr);
    ASSERT_NE(browser_view->toolbar(), nullptr);
    toolbar_button_provider_ = browser_view->toolbar_button_provider();
    ASSERT_NE(toolbar_button_provider_, nullptr);
  }

 protected:
  bool is_avatar_button_shown() {
    views::View* button = toolbar_button_provider_->GetAvatarToolbarButton();
    DCHECK(button);
    return button->GetVisible();
  }

 private:
  ToolbarButtonProvider* toolbar_button_provider_;
  DISALLOW_COPY_AND_ASSIGN(BraveToolbarViewTest);
};

IN_PROC_BROWSER_TEST_F(BraveToolbarViewTest,
    AvatarButtonNotShownSingleProfile) {
  EXPECT_EQ(false, is_avatar_button_shown());
}

IN_PROC_BROWSER_TEST_F(BraveToolbarViewTest, AvatarButtonIsShownGuestProfile) {
  // Open a Guest window.
  EXPECT_EQ(1U, BrowserList::GetInstance()->size());
  content::WindowedNotificationObserver browser_creation_observer(
      chrome::NOTIFICATION_BROWSER_OPENED,
      content::NotificationService::AllSources());
  profiles::SwitchToGuestProfile(ProfileManager::CreateCallback());
  base::RunLoop().RunUntilIdle();
  browser_creation_observer.Wait();
  EXPECT_EQ(2U, BrowserList::GetInstance()->size());

  // Retrieve the new Guest profile.
  Profile* guest = g_browser_process->profile_manager()->GetProfileByPath(
      ProfileManager::GetGuestProfilePath());

  // Access the browser with the Guest profile and re-init test for it.
  Browser* browser = chrome::FindAnyBrowser(guest, true);
  EXPECT_TRUE(browser);
  Init(browser);
  EXPECT_EQ(true, is_avatar_button_shown());
}

IN_PROC_BROWSER_TEST_F(BraveToolbarViewTest,
    AvatarButtonIsShownMultipleProfiles) {
  // Should not be shown in first profile, at first
  EXPECT_EQ(false, is_avatar_button_shown());

  // Create an additional profile.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ProfileAttributesStorage& storage =
      profile_manager->GetProfileAttributesStorage();
  base::FilePath current_profile_path = browser()->profile()->GetPath();
  base::FilePath new_path = profile_manager->GenerateNextProfileDirectoryPath();
  base::RunLoop run_loop;
  profile_manager->CreateProfileAsync(
      new_path, base::Bind(&OnUnblockOnProfileCreation, &run_loop),
      std::u16string(), std::string());
  run_loop.Run();
  ASSERT_EQ(2u, storage.GetNumberOfProfiles());
  Profile* new_profile = profile_manager->GetProfileByPath(new_path);

  // check it's now shown in first profile
  EXPECT_EQ(true, is_avatar_button_shown());

  // Open the new profile
  EXPECT_EQ(1U, BrowserList::GetInstance()->size());
  content::WindowedNotificationObserver browser_creation_observer(
      chrome::NOTIFICATION_BROWSER_OPENED,
      content::NotificationService::AllSources());
  profiles::OpenBrowserWindowForProfile(
    ProfileManager::CreateCallback(),
    false, true, true,
    new_profile,
    Profile::CREATE_STATUS_INITIALIZED);
  base::RunLoop().RunUntilIdle();
  browser_creation_observer.Wait();
  EXPECT_EQ(2U, BrowserList::GetInstance()->size());

  // Check it's shown in second profile
  Browser* browser = chrome::FindAnyBrowser(new_profile, true);
  EXPECT_TRUE(browser);
  Init(browser);
  EXPECT_EQ(true, is_avatar_button_shown());
}

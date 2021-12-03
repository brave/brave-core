// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/common/pref_names.h"
#include "brave/components/skus/common/features.h"
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
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "ui/views/view.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"
#include "brave/components/brave_vpn/features.h"
#include "brave/components/brave_vpn/pref_names.h"
#endif

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
  BraveToolbarViewTest() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
#endif
  }
  BraveToolbarViewTest(const BraveToolbarViewTest&) = delete;
  BraveToolbarViewTest& operator=(const BraveToolbarViewTest&) = delete;
  ~BraveToolbarViewTest() override = default;

  void SetUpOnMainThread() override { Init(browser()); }

  void Init(Browser* browser) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
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
  raw_ptr<ToolbarButtonProvider> toolbar_button_provider_ = nullptr;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  base::test::ScopedFeatureList scoped_feature_list_;
#endif
};

#if BUILDFLAG(ENABLE_BRAVE_VPN)
IN_PROC_BROWSER_TEST_F(BraveToolbarViewTest, VPNButtonVisibility) {
  auto* browser_view = static_cast<BraveBrowserView*>(
      BrowserView::GetBrowserViewForBrowser(browser()));
  auto* toolbar = static_cast<BraveToolbarView*>(browser_view->toolbar());
  auto* prefs = browser()->profile()->GetPrefs();

  // Button is visible by default.
  EXPECT_TRUE(prefs->GetBoolean(brave_vpn::prefs::kBraveVPNShowButton));
  EXPECT_TRUE(toolbar->brave_vpn_button()->GetVisible());
  EXPECT_EQ(browser_view->GetAnchorViewForBraveVPNPanel(),
            toolbar->brave_vpn_button());

  // Hide button.
  prefs->SetBoolean(brave_vpn::prefs::kBraveVPNShowButton, false);
  EXPECT_FALSE(toolbar->brave_vpn_button()->GetVisible());
  EXPECT_EQ(browser_view->GetAnchorViewForBraveVPNPanel(),
            static_cast<views::View*>(toolbar->app_menu_button()));
}
#endif

IN_PROC_BROWSER_TEST_F(BraveToolbarViewTest,
                       AvatarButtonNotShownSingleProfile) {
  EXPECT_EQ(false, is_avatar_button_shown());
}

IN_PROC_BROWSER_TEST_F(BraveToolbarViewTest, AvatarButtonIsShownGuestProfile) {
  // Open a Guest window.
  EXPECT_EQ(1U, BrowserList::GetInstance()->size());
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
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
      new_path, base::BindRepeating(&OnUnblockOnProfileCreation, &run_loop));
  run_loop.Run();
  ASSERT_EQ(2u, storage.GetNumberOfProfiles());
  Profile* new_profile = profile_manager->GetProfileByPath(new_path);

  // check it's now shown in first profile
  EXPECT_EQ(true, is_avatar_button_shown());

  // Open the new profile
  EXPECT_EQ(1U, BrowserList::GetInstance()->size());
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  profiles::OpenBrowserWindowForProfile(ProfileManager::CreateCallback(), false,
                                        true, true, new_profile,
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

// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/toolbar/bookmark_button.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/skus/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
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
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "ui/views/view.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

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

  // InProcessBrowserTest override
  void SetUpOnMainThread() override { Init(browser()); }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    provider_.SetDefaultReturns(
        /*is_initialization_complete_return=*/true,
        /*is_first_policy_load_complete_return=*/true);
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
  }

  void BlockVPNByPolicy(bool value) {
    policy::PolicyMap policies;
    policies.Set(policy::key::kBraveVPNDisabled, policy::POLICY_LEVEL_MANDATORY,
                 policy::POLICY_SCOPE_MACHINE, policy::POLICY_SOURCE_PLATFORM,
                 base::Value(value), nullptr);
    provider_.UpdateChromePolicy(policies);
    EXPECT_EQ(
        brave_vpn::IsBraveVPNDisabledByPolicy(browser()->profile()->GetPrefs()),
        value);
  }
#endif
  void Init(Browser* browser) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    ASSERT_NE(browser_view, nullptr);

    toolbar_view_ = static_cast<BraveToolbarView*>(browser_view->toolbar());
    ASSERT_NE(toolbar_view_, nullptr);

    toolbar_button_provider_ = browser_view->toolbar_button_provider();
    ASSERT_NE(toolbar_button_provider_, nullptr);
  }

 protected:
  bool is_avatar_button_shown() {
    views::View* button = toolbar_button_provider_->GetAvatarToolbarButton();
    DCHECK(button);
    return button->GetVisible();
  }

  bool is_bookmark_button_shown() {
    BookmarkButton* bookmark_button = toolbar_view_->bookmark_button();
    DCHECK(bookmark_button);
    return bookmark_button->GetVisible();
  }

  raw_ptr<ToolbarButtonProvider> toolbar_button_provider_ = nullptr;
  raw_ptr<BraveToolbarView> toolbar_view_ = nullptr;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  testing::NiceMock<policy::MockConfigurationPolicyProvider> provider_;
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
  // Show button.
  prefs->SetBoolean(brave_vpn::prefs::kBraveVPNShowButton, true);
  EXPECT_TRUE(toolbar->brave_vpn_button()->GetVisible());
  BlockVPNByPolicy(true);
  EXPECT_FALSE(toolbar->brave_vpn_button()->GetVisible());
  EXPECT_TRUE(prefs->GetBoolean(brave_vpn::prefs::kBraveVPNShowButton));
  BlockVPNByPolicy(false);
  EXPECT_TRUE(toolbar->brave_vpn_button()->GetVisible());
  EXPECT_TRUE(prefs->GetBoolean(brave_vpn::prefs::kBraveVPNShowButton));
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
  profiles::SwitchToGuestProfile(base::DoNothing());
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
  Profile& new_profile =
      profiles::testing::CreateProfileSync(profile_manager, new_path);
  ASSERT_EQ(2u, storage.GetNumberOfProfiles());

  // check it's now shown in first profile
  EXPECT_EQ(true, is_avatar_button_shown());

  // Open the new profile
  EXPECT_EQ(1U, BrowserList::GetInstance()->size());
  ui_test_utils::BrowserChangeObserver browser_creation_observer(
      nullptr, ui_test_utils::BrowserChangeObserver::ChangeType::kAdded);
  profiles::OpenBrowserWindowForProfile(base::DoNothing(), false, true, true,
                                        &new_profile);
  base::RunLoop().RunUntilIdle();
  browser_creation_observer.Wait();
  EXPECT_EQ(2U, BrowserList::GetInstance()->size());

  // Check it's shown in second profile
  Browser* browser = chrome::FindAnyBrowser(&new_profile, true);
  EXPECT_TRUE(browser);
  Init(browser);
  EXPECT_EQ(true, is_avatar_button_shown());

  // Check avatar is positioned at the right before app menu button.
  views::View* avatar = toolbar_button_provider_->GetAvatarToolbarButton();
  ASSERT_TRUE(!!avatar);
  views::View* container = avatar->parent();
  ASSERT_TRUE(!!container);
  views::View* app_menu = toolbar_button_provider_->GetAppMenuButton();
  ASSERT_TRUE(!!app_menu);
  EXPECT_EQ(container->GetIndexOf(avatar).value(),
            container->GetIndexOf(app_menu).value() - 1ul);
}

IN_PROC_BROWSER_TEST_F(BraveToolbarViewTest,
                       BookmarkButtonCanBeToggledWithPref) {
  auto* prefs = browser()->profile()->GetPrefs();

  // By default, the button should be shown.
  EXPECT_TRUE(prefs->GetBoolean(kShowBookmarksButton));
  EXPECT_TRUE(is_bookmark_button_shown());

  // Hide button.
  prefs->SetBoolean(kShowBookmarksButton, false);
  EXPECT_FALSE(is_bookmark_button_shown());

  // Reshowing the button should also work.
  prefs->SetBoolean(kShowBookmarksButton, true);
  EXPECT_TRUE(is_bookmark_button_shown());
}

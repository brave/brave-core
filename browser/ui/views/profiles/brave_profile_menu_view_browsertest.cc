/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "chrome/browser/ui/views/profiles/profile_menu_coordinator.h"
#include "chrome/browser/ui/views/profiles/profile_menu_view_base.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/label.h"
#include "ui/views/test/widget_activation_waiter.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/widget/widget.h"

class BraveProfileMenuViewTest : public InProcessBrowserTest {
 public:
  BraveProfileMenuViewTest() = default;
  BraveProfileMenuViewTest(const BraveProfileMenuViewTest&) = delete;
  BraveProfileMenuViewTest& operator=(const BraveProfileMenuViewTest&) = delete;
  ~BraveProfileMenuViewTest() override = default;

 protected:
  void ClickAvatarToolbarButton(AvatarToolbarButton* avatar_toolbar_button) {
    ui::MouseEvent press(ui::EventType::kMousePressed, gfx::Point(),
                         gfx::Point(), ui::EventTimeForNow(),
                         ui::EF_LEFT_MOUSE_BUTTON, 0);
    ui::MouseEvent release(ui::EventType::kMouseReleased, gfx::Point(),
                           gfx::Point(), ui::EventTimeForNow(),
                           ui::EF_LEFT_MOUSE_BUTTON, 0);
    avatar_toolbar_button->OnMousePressed(press);
    avatar_toolbar_button->OnMouseReleased(release);
  }

  void WaitForMenuToBeActive(ProfileMenuViewBase* profile_menu_view) {
    ASSERT_TRUE(profile_menu_view);
    profile_menu_view->set_close_on_deactivate(false);
#if BUILDFLAG(IS_MAC)
    base::RunLoop().RunUntilIdle();
#else
    views::Widget* menu_widget = profile_menu_view->GetWidget();
    ASSERT_TRUE(menu_widget);
    if (menu_widget->CanActivate()) {
      views::test::WaitForWidgetActive(menu_widget, /*active=*/true);
    } else {
      LOG(ERROR) << "menu_widget can not be activated";
    }
#endif
  }

  ProfileMenuViewBase* profile_menu_view(Browser* browser) {
    auto* coordinator = browser->GetFeatures().profile_menu_coordinator();
    return coordinator ? coordinator->GetProfileMenuViewBaseForTesting()
                       : nullptr;
  }

  void OpenProfileMenu(Browser* browser) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    AvatarToolbarButton* avatar_toolbar_button =
        browser_view->toolbar_button_provider()->GetAvatarToolbarButton();
    views::test::WidgetVisibleWaiter(avatar_toolbar_button->GetWidget()).Wait();
    ASSERT_TRUE(avatar_toolbar_button);
    ClickAvatarToolbarButton(avatar_toolbar_button);
    ASSERT_NO_FATAL_FAILURE(WaitForMenuToBeActive(profile_menu_view(browser)));
    auto* coordinator = browser->GetFeatures().profile_menu_coordinator();
    EXPECT_TRUE(coordinator->IsShowing());
  }

  std::u16string GetProfileName(Profile* profile) {
    ProfileAttributesEntry* profile_attributes =
        g_browser_process->profile_manager()
            ->GetProfileAttributesStorage()
            .GetProfileAttributesWithPath(profile->GetPath());
    return profile_attributes->GetName();
  }

  void CheckIdentity(Browser* browser) {
    ProfileMenuViewBase* menu = profile_menu_view(browser);
    // Profile image and title container
    EXPECT_EQ(2u, menu->identity_info_container_->children().size());
    // Profile image has no children
    EXPECT_EQ(0u,
              menu->identity_info_container_->children()[0]->children().size());
    // Title container has no children
    const auto* title_container_view =
        menu->identity_info_container_->children()[1].get();
    EXPECT_EQ(0u, title_container_view->children().size());
    if (!browser->profile()->IsGuestSession()) {
      EXPECT_EQ(
          GetProfileName(browser->profile()),
          static_cast<const views::Label*>(title_container_view)->GetText());
    }
  }

  void CreateAdditionalProfile() {
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    size_t starting_number_of_profiles = profile_manager->GetNumberOfProfiles();

    base::FilePath new_path =
        profile_manager->GenerateNextProfileDirectoryPath();
    profiles::testing::CreateProfileSync(profile_manager, new_path);
    EXPECT_EQ(starting_number_of_profiles + 1,
              profile_manager->GetNumberOfProfiles());
  }
};

IN_PROC_BROWSER_TEST_F(BraveProfileMenuViewTest, TestCurrentProfileView) {
  // Avatar menu button is not visible unless we have more than one profile.
  CreateAdditionalProfile();

  OpenProfileMenu(browser());
  CheckIdentity(browser());
}

IN_PROC_BROWSER_TEST_F(BraveProfileMenuViewTest, OpenGuestWindowProfile) {
  // Open a Guest window.
  EXPECT_EQ(1U, BrowserList::GetInstance()->size());
  profiles::SwitchToGuestProfile(base::DoNothing());
  Browser* guest_browser = ui_test_utils::WaitForBrowserToOpen();
  EXPECT_EQ(2U, BrowserList::GetInstance()->size());

  OpenProfileMenu(guest_browser);
  CheckIdentity(guest_browser);
}

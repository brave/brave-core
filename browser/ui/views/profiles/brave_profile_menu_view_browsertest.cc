/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "chrome/browser/ui/views/profiles/profile_menu_view_base.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/label.h"

class BraveProfileMenuViewTest : public InProcessBrowserTest {
 public:
  BraveProfileMenuViewTest() = default;
  BraveProfileMenuViewTest(const BraveProfileMenuViewTest&) = delete;
  BraveProfileMenuViewTest& operator=(const BraveProfileMenuViewTest&) = delete;
  ~BraveProfileMenuViewTest() override = default;

 protected:
  AvatarToolbarButton* avatar_button() {
    BrowserView* browser_view =
        BrowserView::GetBrowserViewForBrowser(browser());
    AvatarToolbarButton* button =
        browser_view->toolbar_button_provider()->GetAvatarToolbarButton();
    DCHECK(button);
    return button;
  }

  void OpenProfileMenuView() {
    ui::MouseEvent press(ui::ET_MOUSE_PRESSED, gfx::Point(), gfx::Point(),
                         ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0);
    ui::MouseEvent release(ui::ET_MOUSE_RELEASED, gfx::Point(), gfx::Point(),
                           ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0);
    avatar_button()->OnMousePressed(press);
    avatar_button()->OnMousePressed(release);
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(ProfileMenuViewBase::IsShowing());
  }

  ProfileMenuViewBase* profile_menu() {
    ProfileMenuViewBase* bubble = ProfileMenuViewBase::GetBubbleForTesting();
    DCHECK(bubble);
    return bubble;
  }

  std::u16string GetProfileName() {
    Profile* profile = browser()->profile();
    ProfileAttributesEntry* profile_attributes =
        g_browser_process->profile_manager()
            ->GetProfileAttributesStorage()
            .GetProfileAttributesWithPath(profile->GetPath());
    return profile_attributes->GetName();
  }

  void CheckIdentity() {
    ProfileMenuViewBase* menu = profile_menu();
    // Profile image and title container.
    EXPECT_EQ(2u, menu->identity_info_container_->children().size());
    // Profile image should have 1 child - header and image container.
    EXPECT_EQ(1u,
              menu->identity_info_container_->children()[0]->children().size());
    // Title container should have 1 child - title, which is the profile name.
    const auto* title_container_view =
        menu->identity_info_container_->children()[1];
    EXPECT_EQ(1u, title_container_view->children().size());
    const auto* title_view = title_container_view->children()[0];
    EXPECT_EQ(GetProfileName(),
              static_cast<const views::Label*>(title_view)->GetText());
  }
};

IN_PROC_BROWSER_TEST_F(BraveProfileMenuViewTest, TestCurrentProfileView) {
  OpenProfileMenuView();
  CheckIdentity();
}

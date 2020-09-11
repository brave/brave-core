/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "chrome/browser/ui/views/profiles/profile_menu_view_base.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/label_button.h"

class BraveProfileMenuViewTest : public InProcessBrowserTest {
 public:
  BraveProfileMenuViewTest() = default;
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

  void CheckIdentityHasNoText() {
    ProfileMenuViewBase* menu = profile_menu();
    // Profile image and title container.
    EXPECT_EQ(2u, menu->identity_info_container_->children().size());
    // Each should have 0 children.
    for (const auto* view : menu->identity_info_container_->children()) {
      EXPECT_EQ(0u, view->children().size());
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveProfileMenuViewTest);
};

IN_PROC_BROWSER_TEST_F(BraveProfileMenuViewTest, TestCurrentProfileView) {
  OpenProfileMenuView();
  CheckIdentityHasNoText();
}

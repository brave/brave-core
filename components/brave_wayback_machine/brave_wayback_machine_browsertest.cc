/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/location_bar/icon_label_bubble_view.h"
#include "chrome/browser/ui/views/page_action/test_support/page_action_test_support.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "net/http/http_status_code.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/test/button_test_api.h"

class BraveWaybackMachineTest : public InProcessBrowserTest {
 protected:
  BraveWaybackMachineTabHelper* GetTabHelper() {
    return BraveWaybackMachineTabHelper::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

  IconLabelBubbleView* GetIcon() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    auto* button_provider = browser_view->toolbar_button_provider();
    return page_actions::GetIconLabelBubbleViewForTesting(
        button_provider->GetPageActionViewInterface(kActionShowWaybackMachine),
        kActionShowWaybackMachine);
  }

  bool ShouldCheckWaybackMachine(int response_code) {
    return GetTabHelper()->ShouldCheckWaybackMachine(response_code);
  }

  void SetWaybackState(WaybackState state) {
    GetTabHelper()->SetWaybackState(state);
  }
};

IN_PROC_BROWSER_TEST_F(BraveWaybackMachineTest, BubbleLaunchTest) {
  EXPECT_FALSE(ShouldCheckWaybackMachine(net::HTTP_OK));
  EXPECT_TRUE(ShouldCheckWaybackMachine(net::HTTP_NOT_FOUND));

  auto* icon = GetIcon();
  EXPECT_FALSE(icon->GetVisible());

  SetWaybackState(WaybackState::kNeedToCheck);
  EXPECT_TRUE(icon->GetVisible());

  // Check bubble is launched.
  views::test::ButtonTestApi(icon).NotifyClick(
      ui::MouseEvent(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                     ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                     ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_TRUE(GetTabHelper()->active_window().has_value());
}

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

using BraveWaybackMachineTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveWaybackMachineTest, BubbleLaunchTest) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* button_provider = browser_view->toolbar_button_provider();

  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  BraveWaybackMachineTabHelper* tab_helper =
      BraveWaybackMachineTabHelper::FromWebContents(contents);
  EXPECT_FALSE(tab_helper->ShouldCheckWaybackMachine(net::HTTP_OK));
  EXPECT_TRUE(tab_helper->ShouldCheckWaybackMachine(net::HTTP_NOT_FOUND));

  auto* icon = page_actions::GetIconLabelBubbleViewForTesting(
      button_provider->GetPageActionViewInterface(kActionShowWaybackMachine),
      kActionShowWaybackMachine);
  EXPECT_FALSE(icon->GetVisible());

  tab_helper->SetWaybackState(WaybackState::kNeedToCheck);
  EXPECT_TRUE(icon->GetVisible());

  // Check bubble is launched.
  views::test::ButtonTestApi(icon).NotifyClick(
      ui::MouseEvent(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                     ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON,
                     ui::EF_LEFT_MOUSE_BUTTON));
  EXPECT_TRUE(tab_helper->active_window().has_value());
}

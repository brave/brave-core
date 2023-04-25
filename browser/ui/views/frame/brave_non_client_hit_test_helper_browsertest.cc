/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/reload_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/hit_test.h"

using BraveNonClientHitTestHelperBrowserTest = InProcessBrowserTest;

// TODO(sko) It might be good to have resizable area tests. But testing it
// is pretty flaky depending on platforms.
IN_PROC_BROWSER_TEST_F(BraveNonClientHitTestHelperBrowserTest, Toolbar) {
  auto* browser_view = static_cast<BrowserView*>(browser()->window());
  auto* toolbar = browser_view->toolbar();
  auto* frame_view = browser_view->frame()->GetFrameView();

  for (auto* view : toolbar->GetChildrenInZOrder()) {
    // When a point is on children view, hit test result will be HTCLIENT.
    // In order to make this test without flakiness, hide theme.
    view->SetVisible(false);
  }

  gfx::Point point = toolbar->GetLocalBounds().CenterPoint();
  views::View::ConvertPointToWidget(toolbar, &point);

  // Dragging a window with the toolbar on it should work.
  EXPECT_EQ(HTCAPTION, frame_view->NonClientHitTest(point));

  // It shouldn't be perceived as a HTCAPTION when it's not visible.
  toolbar->SetVisible(false);
  EXPECT_NE(HTCAPTION, frame_view->NonClientHitTest(point));

  // A coordinate on children of toolbar shouldn't be HTCAPTION so that users
  // can interact with them. Checks a typical child of toolbar as a sanity
  // check.
  toolbar->SetVisible(true);
  point = gfx::Point();

  for (auto* view : toolbar->GetChildrenInZOrder()) {
    view->SetVisible(true);
  }

  views::View::ConvertPointToWidget(toolbar->reload_button(), &point);
  EXPECT_NE(HTCAPTION, frame_view->NonClientHitTest(point));
}

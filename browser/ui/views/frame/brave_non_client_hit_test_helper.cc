/* Copyright (c) 2022 The Brave Authors. All rights reserved.
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_non_client_hit_test_helper.h"

#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/hit_test.h"
#include "ui/views/window/hit_test_utils.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"

namespace brave {

int NonClientHitTest(BrowserView* browser_view, const gfx::Point& point) {
  if (!browser_view->toolbar() || !browser_view->toolbar()->GetVisible())
    return HTNOWHERE;

  return views::GetHitTestComponent(browser_view->toolbar(), point);
}

}  // brave


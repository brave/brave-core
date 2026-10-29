/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/layout_constants.h"

// static
int BraveContentsViewUtil::GetRoundedCornersWebViewMargin(
    const BrowserWindowInterface* browser) {
  return BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
             browser)
             ? kRoundedCornersContentsViewMargin
             : 0;
}

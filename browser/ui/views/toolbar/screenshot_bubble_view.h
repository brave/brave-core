// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_BUBBLE_VIEW_H_

#include <memory>

namespace content {
class WebContents;
}  // namespace content

namespace views {
class View;
class BubbleDialogModelHost;
}  // namespace views

namespace screenshot {

class ScreenshotController;

// Creates and shows a screenshot-options bubble anchored to |anchor|.
// Returns the bubble's widget so the caller can track / close it.
std::unique_ptr<views::BubbleDialogModelHost> ShowScreenshotBubble(
    content::WebContents* web_contents,
    views::View* anchor,
    ScreenshotController* controller);

}  // namespace screenshot

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SCREENSHOT_BUBBLE_VIEW_H_

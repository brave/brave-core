// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_SCREENSHOTS_BRAVE_SCREENSHOTS_UTILS_H_
#define BRAVE_BROWSER_UI_SCREENSHOTS_BRAVE_SCREENSHOTS_UTILS_H_

class Browser;

namespace base {
template <typename T>
class WeakPtr;
}  // namespace base

namespace content {
class WebContents;
}  // namespace content

namespace brave_utils {

void ScreenshotSelectionToClipboard(base::WeakPtr<Browser>);
void ScreenshotViewportToClipboard(base::WeakPtr<content::WebContents>);
void ScreenshotFullPageToClipboard(base::WeakPtr<content::WebContents>);

}  // namespace brave_utils

#endif  // BRAVE_BROWSER_UI_SCREENSHOTS_BRAVE_SCREENSHOTS_UTILS_H_

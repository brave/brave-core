/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

class BrowserWindowInterface;

class BraveContentsViewUtil {
 public:
  // If rounded corners are enabled, returns the additional margin required to
  // get the shadow to display properly. Otherwise 0.
  static int GetRoundedCornersWebViewMargin(
      const BrowserWindowInterface* browser);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

#include <memory>

#include "brave/browser/ui/views/view_shadow.h"

class Browser;
class BrowserWindowInterface;

namespace gfx {
class RoundedCornersF;
}  // namespace gfx

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace views {
class View;
}  // namespace views

class BraveContentsViewUtil {
 public:
  // The distance between main content areas and other UI elements.
  static constexpr int kMarginThickness = 4;

  // The border radius applied to main content areas.
  // Different value per platforms.
  static int GetBorderRadius();

  // The border radius of the corner attached to the corner of the browser
  // window
  static int GetBorderRadiusAroundWindow();

  // Creates a drop shadow for the specified content area view.
  static std::unique_ptr<ViewShadow> CreateShadow(views::View* view);

  // If rounded corners are enabled, returns the additional margin required to
  // get the shadow to display properly. Otherwise 0.
  static int GetRoundedCornersWebViewMargin(Browser* browser);

  // Pass content's tab to |tab| if it needs to consider split view state.
  static gfx::RoundedCornersF GetRoundedCornersForContentsView(
      BrowserWindowInterface* browser_window_interface,
      tabs::TabInterface* tab);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

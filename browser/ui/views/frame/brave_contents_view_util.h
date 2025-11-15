/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

#include <memory>

#include "brave/browser/ui/views/view_shadow.h"

class Browser;

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
  // Different value per platforms.
  static int GetMargin();

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

  // Pass content's tab as |split_tab| if its position in is split view
  // should be considered.
  static gfx::RoundedCornersF GetRoundedCornersForContentsView(
      Browser* browser,
      tabs::TabInterface* split_tab = nullptr);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

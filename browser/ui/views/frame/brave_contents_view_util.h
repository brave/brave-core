/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

#include <memory>

#include "ui/gfx/geometry/rounded_corners_f.h"

class Browser;
class BrowserWindowInterface;

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace ui {
class ColorProvider;
}  // namespace ui

namespace views {
class Border;
}  // namespace views

class BraveContentsViewUtil {
 public:
  // Creates a 1px rounded-rect outline border matching |corner_radii|.
  static std::unique_ptr<views::Border> CreateContentsOutlineBorder(
      const ui::ColorProvider* color_provider,
      const gfx::RoundedCornersF& corner_radii);

  // If rounded corners are enabled, returns the additional margin required to
  // get the shadow to display properly. Otherwise 0.
  static int GetRoundedCornersWebViewMargin(Browser* browser);
  static int GetRoundedCornersWebViewMargin(const Browser* browser);

  // Pass content's tab to |tab| if it needs to consider split view state.
  static gfx::RoundedCornersF GetRoundedCornersForContentsView(
      BrowserWindowInterface* browser_window_interface,
      tabs::TabInterface* tab);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

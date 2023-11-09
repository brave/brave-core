/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

#include <memory>

#include "brave/browser/ui/views/view_shadow.h"

namespace views {
class View;
}

class BraveContentsViewUtil {
 public:
  // The distance between main content areas and other UI elements.
  static constexpr int kMarginThickness = 4;

  // The border radius applied to main content areas.
  static constexpr int kBorderRadius = 4;

  // Creates a drop shadow for the specified content area view.
  static std::unique_ptr<ViewShadow> CreateShadow(views::View* view);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_CONTENTS_VIEW_UTIL_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_ICON_LABEL_BUBBLE_VIEW_GET_HIGHLIGHT_PATH                    \
  const int layout_radius =                                                \
      GetLayoutConstant(LayoutConstant::kLocationBarChildCornerRadius);    \
  return SkPath::RRect(gfx::RectToSkRect(highlight_bounds), layout_radius, \
                       layout_radius);

// Defined as empty to satisfy the macro reference in the upstream
// IconLabelBubbleView::SetImageModel() without injecting additional behavior.
#define BRAVE_ICON_LABEL_BUBBLE_VIEW_SET_IMAGE_MODEL

#include <chrome/browser/ui/views/location_bar/icon_label_bubble_view.cc>
#undef BRAVE_ICON_LABEL_BUBBLE_VIEW_SET_IMAGE_MODEL
#undef BRAVE_ICON_LABEL_BUBBLE_VIEW_GET_HIGHLIGHT_PATH

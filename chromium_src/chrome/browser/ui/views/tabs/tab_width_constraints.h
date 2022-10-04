/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_WIDTH_CONSTRAINTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_WIDTH_CONSTRAINTS_H_

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace gfx {
class Rect;
}  // namespace gfx

#define TransformForPinnednessAndOpenness                                  \
  TransformForPinnednessAndOpenness_UnUsed() { return {}; }                \
  friend std::vector<gfx::Rect> CalculateVerticalTabBounds(                \
      const TabLayoutConstants& layout_constants,                          \
      const std::vector<TabWidthConstraints>& tabs,                        \
      absl::optional<int> width);                                          \
                                                                           \
 public:                                                                   \
  void set_is_tab_in_group(bool in_group) { is_tab_in_group_ = in_group; } \
  bool is_tab_in_group() const { return is_tab_in_group_; }                \
                                                                           \
 private:                                                                  \
  bool is_tab_in_group_ = false;                                           \
  float TransformForPinnednessAndOpenness

#include "src/chrome/browser/ui/views/tabs/tab_width_constraints.h"

#undef TransformForPinnednessAndOpenness

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_WIDTH_CONSTRAINTS_H_

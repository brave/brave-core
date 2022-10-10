/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_

#include <vector>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace gfx {
class Rect;
}  // namespace gfx

class TabStripLayoutHelper;
class TabWidthConstraints;
class TabWidthConstraints;
struct TabLayoutConstants;

namespace tabs {

std::vector<gfx::Rect> CalculateVerticalTabBounds(
    const TabLayoutConstants& layout_constants,
    const std::vector<TabWidthConstraints>& tabs,
    absl::optional<int> width);

// This function's definition is in chromium_src/../tab_strip_layout_helper.cc
// Please see the comment there for why.
bool FillGroupInfo(std::vector<TabWidthConstraints>& tab_widths,
                   TabStripLayoutHelper& helper);

}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_

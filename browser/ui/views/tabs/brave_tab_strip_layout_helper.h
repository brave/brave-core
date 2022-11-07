/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_

#include <vector>

#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace gfx {
class Rect;
}  // namespace gfx

class TabStripLayoutHelper;
class TabWidthConstraints;
class TabWidthConstraints;
struct TabLayoutConstants;

namespace tabs {

constexpr int kVerticalTabMinWidth = SidebarButtonView::kSidebarButtonSize;
constexpr int kVerticalTabHeight = SidebarButtonView::kSidebarButtonSize;

std::vector<gfx::Rect> CalculateVerticalTabBounds(
    const TabLayoutConstants& layout_constants,
    const std::vector<TabWidthConstraints>& tabs,
    absl::optional<int> width);

}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_HELPER_H_

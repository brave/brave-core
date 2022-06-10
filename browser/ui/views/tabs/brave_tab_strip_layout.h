/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_H_

#include <vector>

#include "chrome/browser/ui/views/tabs/tab_strip_layout_types.h"
#include "chrome/browser/ui/views/tabs/tab_width_constraints.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace gfx {
class Rect;
}  // namespace gfx

std::vector<gfx::Rect> CalculateVerticalTabBounds(
    const TabLayoutConstants& layout_constants,
    const std::vector<TabWidthConstraints>& tabs,
    absl::optional<int> width);

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_LAYOUT_H_

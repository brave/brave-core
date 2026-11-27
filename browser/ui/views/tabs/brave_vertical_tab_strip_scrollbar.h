/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_VERTICAL_TAB_STRIP_SCROLLBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_VERTICAL_TAB_STRIP_SCROLLBAR_H_

#include <memory>

#include "ui/views/controls/scroll_view.h"

class PrefService;
class VerticalTabStripScrollBar;

namespace tabs {
class VerticalTabStripStateController;
}  // namespace tabs

// Creates a VerticalTabStripScrollBar which follows
// brave_tabs::kVerticalTabsShowScrollbar. The scroll bar updates the vertical
// scroll bar mode of its ScrollView whenever the pref changes.
//
// This is also forward-declared in the chromium_src override of
// tab_strip_view.cc, so that upstream target doesn't depend on this header. The
// linker resolves the definition.
std::unique_ptr<VerticalTabStripScrollBar> CreateBraveVerticalTabStripScrollBar(
    tabs::VerticalTabStripStateController* state_controller,
    PrefService* prefs);

// Returns the mode `scroll_view`'s vertical scroll bar should use when its
// overflow visuals are enabled. Honors the "show scroll bar" pref.
views::ScrollView::ScrollBarMode GetBraveVerticalTabStripScrollBarMode(
    views::ScrollView* scroll_view);

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_VERTICAL_TAB_STRIP_SCROLLBAR_H_

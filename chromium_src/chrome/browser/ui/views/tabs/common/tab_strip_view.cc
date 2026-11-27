// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/tab_strip_view.h"

#include <memory>

#include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_scroll_bar.h"
#include "ui/views/controls/scroll_view.h"

class PrefService;

// Vertical tab strip scroll bar hooks. Defined in
// brave/browser/ui/views/tabs/brave_vertical_tab_strip_scrollbar.cc so that
// this upstream target doesn't depend on Brave targets.
std::unique_ptr<VerticalTabStripScrollBar> CreateBraveVerticalTabStripScrollBar(
    tabs::VerticalTabStripStateController* state_controller,
    PrefService* prefs);

views::ScrollView::ScrollBarMode GetBraveVerticalTabStripScrollBarMode(
    views::ScrollView* scroll_view);

#include <chrome/browser/ui/views/tabs/common/tab_strip_view.cc>

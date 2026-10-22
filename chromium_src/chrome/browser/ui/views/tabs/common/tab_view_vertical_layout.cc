// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/tab_view_vertical_layout.h"

// The tree tab toggle button takes the trailing slot, i.e. the close button's
// slot, when it's visible.
#define BRAVE_TAB_VIEW_VERTICAL_LAYOUT_ON_INSTALLED                     \
  if (TabView().tree_toggle_button_) {                                  \
    tab_children_configs_.insert(                                       \
        tab_children_configs_.begin(),                                  \
        TabChildConfig(TabView().tree_toggle_button_, kIconDesignWidth, \
                       kDefaultPadding, /*align_leading=*/false,        \
                       /*expand=*/false));                              \
  }

#define BRAVE_TAB_VIEW_VERTICAL_LAYOUT_IS_CHILD_VISIBLE \
  if (child_view == TabView().tree_toggle_button_) {    \
    return TabView().ShouldShowTreeToggleButton();      \
  }                                                     \
  if (child_view == TabView().close_button_ &&          \
      TabView().ShouldShowTreeToggleButton()) {         \
    return false;                                       \
  }

#include <chrome/browser/ui/views/tabs/common/tab_view_vertical_layout.cc>

#undef BRAVE_TAB_VIEW_VERTICAL_LAYOUT_IS_CHILD_VISIBLE
#undef BRAVE_TAB_VIEW_VERTICAL_LAYOUT_ON_INSTALLED

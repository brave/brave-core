/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_SET_USE_VERTICAL_TABS \
  void set_use_vertical_tabs(bool vertical) { use_vertical_tabs_ = vertical; }

#define BRAVE_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_USE_VERTICAL_TABS \
  bool use_vertical_tabs_ = true;

#include "src/chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"

#undef BRAVE_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_SET_USE_VERTICAL_TABS
#undef BRAVE_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_HELPER_USE_VERTICAL_TABS
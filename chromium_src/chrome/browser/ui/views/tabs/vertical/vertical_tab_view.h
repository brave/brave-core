// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_VERTICAL_TAB_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_VERTICAL_TAB_VIEW_H_

#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"

// Adding a dummy implementation of `IsTabMuteIndicatorNotClickable`, as
// `VerticalTabView` is not fully implemented yet.
#define IsApparentlyActive                   \
  IsTabMuteIndicatorNotClickable() override; \
  bool IsApparentlyActive

#include <chrome/browser/ui/views/tabs/vertical/vertical_tab_view.h>  // IWYU pragma: export

#undef IsApparentlyActive

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_VERTICAL_TAB_VIEW_H_

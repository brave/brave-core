/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"
#include "brave/browser/ui/views/tabs/features.h"

#define TAB_CONTAINER_START_REMOVE_TAB_ANIMATION_FOR_VERTICAL_TABS           \
  if (tabs::features::ShouldShowVerticalTabs()) {                            \
    target_bounds.set_y(                                                     \
        (former_model_index > 0)                                             \
            ? tabs_view_model_.ideal_bounds(former_model_index - 1).bottom() \
            : 0);                                                            \
  } else  // NOLINT(readability/braces)

#include "src/chrome/browser/ui/views/tabs/tab_container.cc"

#undef TAB_CONTAINER_START_REMOVE_TAB_ANIMATION_FOR_VERTICAL_TABS

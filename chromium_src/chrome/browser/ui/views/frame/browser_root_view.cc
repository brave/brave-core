/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_root_view.h"

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

// Workaround for vertical tabs to work with drag&drop of text/links.
#define ConvertPointToTarget(THIS, TARGET_GETTER, POINT)               \
  if (views::View* target_v = TARGET_GETTER;                           \
      tabs::utils::ShouldShowVerticalTabs(browser_view_->browser()) && \
      (target_v == tabstrip() || !THIS->Contains(target_v))) {         \
    ConvertPointToScreen(THIS, POINT);                                 \
    ConvertPointFromScreen(target_v, POINT);                           \
  } else {                                                             \
    ConvertPointToTarget(THIS, target_v, POINT);                       \
  }

#include "src/chrome/browser/ui/views/frame/browser_root_view.cc"

#undef ConvertPointToTarget

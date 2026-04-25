/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"

// Currently, `NewTabButton::kButtonSize` is used to calculate tab strip button
// borders. Since the size of buttons varies depending upon the tabs update
// feature flag, replace the `kButtonSize` identifier with a call to the static
// `BraveNewTabButton::GetButtonSize` function.
#define kButtonSize GetButtonSize()

#define BrowserTabStripController BraveBrowserTabStripController
#define NewTabButton BraveNewTabButton
#define TabHoverCardController BraveTabHoverCardController
#include <chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.cc>
#undef TabHoverCardController
#undef NewTabButton
#undef BrowserTabStripController
#undef kButtonSize

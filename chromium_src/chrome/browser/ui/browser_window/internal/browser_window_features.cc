/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "brave/browser/ui/brave_browser_actions.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_browser_web_contents_delegate.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"

#include <chrome/browser/ui/browser_window/internal/browser_window_features.cc>

const SidePanelUI* BrowserWindowFeatures_ChromiumImpl::side_panel_ui() const {
  return const_cast<BrowserWindowFeatures_ChromiumImpl*>(this)->side_panel_ui();
}

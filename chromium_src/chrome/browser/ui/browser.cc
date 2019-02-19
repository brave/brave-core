/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/brave_browser_location_bar_model_delegate.h"

#define BrowserContentSettingBubbleModelDelegate \
  BraveBrowserContentSettingBubbleModelDelegate
#define BrowserCommandController BraveBrowserCommandController
#define BrowserLocationBarModelDelegate BraveBrowserLocationBarModelDelegate
#include "../../../../../chrome/browser/ui/browser.cc"  // NOLINT
#undef BrowserContentSettingBubbleModelDelegate
#undef BrowserCommandController
#undef BrowserLocationBarModelDelegate

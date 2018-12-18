/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/components/omnibox/browser/brave_location_bar_model_impl.h"

#define LocationBarModelImpl BraveLocationBarModelImpl
#define BrowserContentSettingBubbleModelDelegate BraveBrowserContentSettingBubbleModelDelegate
#define BrowserCommandController BraveBrowserCommandController
#include "../../../../../chrome/browser/ui/browser.cc"
#undef BrowserContentSettingBubbleModelDelegate
#undef BrowserCommandController
#undef LocationBarModelImpl

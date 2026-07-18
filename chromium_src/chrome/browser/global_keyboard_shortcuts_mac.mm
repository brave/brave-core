/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/global_keyboard_shortcuts_mac.h"

#include "base/feature_list.h"
#include "brave/components/commands/common/features.h"

#define CommandForKeyEvent CommandForKeyEvent_ChromiumImpl
#include <chrome/browser/global_keyboard_shortcuts_mac.mm>
#undef CommandForKeyEvent

CommandForKeyEventResult CommandForKeyEvent(NSEvent* event) {
  CommandForKeyEventResult result = CommandForKeyEvent_ChromiumImpl(event);
  if (result.found() && !result.from_main_menu &&
      base::FeatureList::IsEnabled(commands::features::kBraveCommands)) {
    // When kBraveCommands is enabled, every profile's shortcuts are managed by
    // commands::AcceleratorService, which registers the shortcuts that aren't
    // backed by a main menu item (Ctrl+PageUp/PageDown, Ctrl+Tab, Cmd+1..9,
    // ...) with the browser's FocusManager, so that users can customize or
    // remove them (brave://settings/system/shortcuts). Suppress the static
    // table dispatch to avoid double handling and to honor customizations of
    // these shortcuts.
    return NoCommand();
  }
  return result;
}

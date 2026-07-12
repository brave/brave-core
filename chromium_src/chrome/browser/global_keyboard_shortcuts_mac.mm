/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/global_keyboard_shortcuts_mac.h"

#include "base/feature_list.h"
#include "brave/browser/ui/commands/accelerator_service_factory.h"
#include "brave/components/commands/common/features.h"
#include "chrome/browser/profiles/profile.h"

#define CommandForKeyEvent CommandForKeyEvent_ChromiumImpl
#include <chrome/browser/global_keyboard_shortcuts_mac.mm>
#undef CommandForKeyEvent

namespace {

// Whether the active profile's shortcuts are managed by
// commands::AcceleratorService, which registers the shortcuts that aren't
// backed by a main menu item (Ctrl+PageUp/PageDown, Ctrl+Tab, Cmd+1..9, ...)
// with the browser's FocusManager, so that users can customize or remove them
// (brave://settings/system/shortcuts). Some profiles (e.g. Guest) have no
// service and keep the upstream static table behavior.
bool ActiveProfileUsesConfigurableShortcuts() {
  if (!base::FeatureList::IsEnabled(commands::features::kBraveCommands)) {
    return false;
  }

  Profile* profile = AppController.sharedController.lastProfileIfLoaded;
  return profile &&
         commands::AcceleratorServiceFactory::GetForContext(profile);
}

}  // namespace

CommandForKeyEventResult CommandForKeyEvent(NSEvent* event) {
  CommandForKeyEventResult result = CommandForKeyEvent_ChromiumImpl(event);
  if (result.found() && !result.from_main_menu &&
      ActiveProfileUsesConfigurableShortcuts()) {
    // Suppress the static table dispatch to avoid double handling and to
    // honor customizations of these shortcuts.
    return NoCommand();
  }
  return result;
}

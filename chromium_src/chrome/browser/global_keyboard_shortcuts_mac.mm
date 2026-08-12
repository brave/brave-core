/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/global_keyboard_shortcuts_mac.h"

#include "base/feature_list.h"
#include "brave/components/commands/common/features.h"

namespace {

// When kBraveCommands is enabled, every profile's shortcuts are managed by
// commands::AcceleratorService, which registers the shortcuts that aren't
// backed by a main menu item (Ctrl+PageUp/PageDown, Ctrl+Tab, Cmd+1..9, ...)
// with the browser's FocusManager, so that users can customize or remove them
// (brave://settings/system/shortcuts). Suppressing the static table dispatch
// for such a command avoids double handling and honors customizations of
// these shortcuts. Called from CommandForKeyEvent via a plaster rewrite (see
// rewrite/chrome/browser/global_keyboard_shortcuts_mac.mm.yaml).
// Note: `result` is taken by value because `found()` is not const-qualified.
bool ShouldSuppressStaticTableCommand(CommandForKeyEventResult result) {
  return result.found() && !result.from_main_menu &&
         base::FeatureList::IsEnabled(commands::features::kBraveCommands);
}

}  // namespace

#include <chrome/browser/global_keyboard_shortcuts_mac.mm>

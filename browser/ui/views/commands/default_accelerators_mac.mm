// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/commands/default_accelerators_mac.h"

#import <Cocoa/Cocoa.h>
#include <optional>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/app/command_utils.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/global_keyboard_shortcuts_mac.h"
#include "chrome/browser/ui/cocoa/accelerators_cocoa.h"
#include "ui/events/cocoa/cocoa_event_utils.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_code_conversion_mac.h"

static_assert(__OBJC__);
static_assert(BUILDFLAG(IS_MAC));

namespace commands {

namespace {

bool CanConvertToAcceleratorMapping(int command_id) {
  if (command_id == 0) {
    return false;
  }

  return base::Contains(commands::GetCommands(), command_id);
}

bool CanConvertToAcceleratorMapping(NSMenuItem* item) {
  if (item.hasSubmenu) {
    return false;
  }

  if (auto command_id = static_cast<int>(item.tag);
      !CanConvertToAcceleratorMapping(command_id)) {
    return false;
  }

  NSString* keyEquivalent = item.keyEquivalent;
  return keyEquivalent != nil && [keyEquivalent length] > 0 &&
         [[keyEquivalent uppercaseString] length] > 0;
}

std::optional<AcceleratorMapping> ToAcceleratorMapping(NSMenuItem* item) {
  bool keyEquivalentLocalizationEnabled = NO;
  bool keyEquivalentMirroringEnabled = NO;

  if (@available(macos 12.0, *)) {
    keyEquivalentLocalizationEnabled =
        item.allowsAutomaticKeyEquivalentLocalization;
    keyEquivalentMirroringEnabled = item.allowsAutomaticKeyEquivalentMirroring;

    // We can't parse keyEquivalent into keycode properly when it's unicode
    // character. So before starting parsing, disable l10n for a while.
    // https://github.com/brave/brave-browser/issues/31770
    if (keyEquivalentLocalizationEnabled) {
      // Setting this to NO will change allowsAutomaticKeyEquivalentMirroring to
      // NO too.
      // https://developer.apple.com/documentation/appkit/nsmenuitem/3787554-allowsautomatickeyequivalentloca?language=objc
      item.allowsAutomaticKeyEquivalentLocalization = NO;
    }
  }

  if (!CanConvertToAcceleratorMapping(item)) {
    if (@available(macos 12.0, *)) {
      item.allowsAutomaticKeyEquivalentLocalization =
          keyEquivalentLocalizationEnabled;
      item.allowsAutomaticKeyEquivalentMirroring =
          keyEquivalentMirroringEnabled;
    }
    return std::nullopt;
  }

  NSString* keyEquivalent = item.keyEquivalent;
  DVLOG(2) << __FUNCTION__ << item.tag << " > "
           << base::SysNSStringToUTF16(keyEquivalent);

  // The ui::KeybaordCode only contains the uppercase characters, we should
  // capitalize them.
  // https://source.chromium.org/chromium/chromium/src/+/main:ui/events/keycodes/keyboard_codes_win.h;l=63;drc=3e1a26c44c024d97dc9a4c09bbc6a2365398ca2c
  const unsigned short charCode =
      [[keyEquivalent uppercaseString] characterAtIndex:0];
  const int macKeyCode = ui::MacKeyCodeForWindowsKeyCode(
      static_cast<ui::KeyboardCode>(charCode), /*flags=*/0,
      /*us_keyboard_shifted_character=*/nullptr,
      /*keyboard_character=*/nullptr);

  NSEvent* keyEvent = [NSEvent keyEventWithType:NSEventTypeKeyDown
                                       location:NSZeroPoint
                                  modifierFlags:0
                                      timestamp:0
                                   windowNumber:0
                                        context:nil
                                     characters:keyEquivalent
                    charactersIgnoringModifiers:keyEquivalent
                                      isARepeat:NO
                                        keyCode:macKeyCode];

  auto modifiers = ui::EventFlagsFromModifiers(item.keyEquivalentModifierMask);
  if (item.keyEquivalentModifierMask & NSEventModifierFlagFunction) {
    // ui::EventFlagsFromModifiers doesn't handle the 'Function key.
    modifiers |= ui::EF_FUNCTION_DOWN;
  }

  if (![keyEquivalent isEqualToString:[keyEquivalent lowercaseString]]) {
    // Shift key could be omitted in |item.keyEquivalentModifierMask| even
    // if it's needed. In this case |item.keyEquivalent| can be already
    // shifted/capitalized. So we should add it manually.
    modifiers |= ui::EF_SHIFT_DOWN;
  }

  const auto command_id = static_cast<int>(item.tag);
  // "Close Tab" and "Close Window" item's shortcuts are statically the same.
  // They are dynamically changed based on the context. e.g. has tab or not,
  // is pwa or not. And also the value is hard coded.
  // https://github.com/chromium/chromium/blob/299385e09d41d5ce3abd434879b5f9b0a8880cd7/chrome/browser/app_controller_mac.mm#L772
  // Here we're to provide brave commands, manually add shift modifier for
  // the "Close Window" item.
  // TODO(sko) As the shortcut is hard coded in the file above, we might need to
  // forbid to adjust default key for IDC_CLOSE_TAB and IDC_CLOSE_WINDOW.
  if (command_id == IDC_CLOSE_WINDOW &&
      (modifiers == ui::EF_COMMAND_DOWN &&
       [keyEquivalent isEqualToString:@"w"])) {
    modifiers |= ui::EF_SHIFT_DOWN;
  }

  if (@available(macos 12.0, *)) {
    item.allowsAutomaticKeyEquivalentLocalization =
        keyEquivalentLocalizationEnabled;
    item.allowsAutomaticKeyEquivalentMirroring = keyEquivalentMirroringEnabled;
  }

  return AcceleratorMapping{.keycode = ui::KeyboardCodeFromNSEvent(keyEvent),
                            .modifiers = modifiers,
                            .command_id = command_id};
}

AcceleratorMapping ToAcceleratorMapping(const KeyboardShortcutData& data) {
  int modifiers = 0;
  if (data.command_key) {
    modifiers |= ui::EF_COMMAND_DOWN;
  }
  if (data.shift_key) {
    modifiers |= ui::EF_SHIFT_DOWN;
  }
  if (data.cntrl_key) {
    modifiers |= ui::EF_CONTROL_DOWN;
  }
  if (data.opt_key) {
    modifiers |= ui::EF_ALT_DOWN;
  }

  return {.keycode = ui::KeyboardCodeFromKeyCode(data.vkey_code),
          .modifiers = modifiers,
          .command_id = data.chrome_command};
}

AcceleratorMapping ToAcceleratorMapping(int command_id,
                                        const ui::Accelerator& accelerator) {
  return {.keycode = accelerator.key_code(),
          .modifiers = accelerator.modifiers(),
          .command_id = command_id};
}

void AccumulateAcceleratorsRecursively(
    base::flat_map<int /*command_id*/, std::vector<AcceleratorMapping>>*
        accelerators,
    NSMenu* menu) {
  CHECK(menu);

  for (NSMenuItem* item in [menu itemArray]) {
    if (auto mapping = ToAcceleratorMapping(item)) {
      (*accelerators)[static_cast<int>(item.tag)].push_back(*mapping);
    }

    if (NSMenu* submenu = [item submenu];
        submenu && submenu != [NSApp servicesMenu]) {
      AccumulateAcceleratorsRecursively(accelerators, submenu);
    }
  }
}

}  // namespace

std::vector<AcceleratorMapping> GetGlobalAccelerators() {
  base::flat_map<int /*command_id*/, std::vector<AcceleratorMapping>>
      accelerator_map;

  // Get dynamic items from main menu.
  AccumulateAcceleratorsRecursively(&accelerator_map, [NSApp mainMenu]);

  // Get items that are not displayed but global.
  for (const auto& shortcut_data : GetShortcutsNotPresentInMainMenu()) {
    if (!CanConvertToAcceleratorMapping(shortcut_data.chrome_command)) {
      DVLOG(2) << " NOT IN COMMANDS : " << shortcut_data.chrome_command;
      continue;
    }

    DVLOG(2) << "IN COMMANDS : " << shortcut_data.chrome_command;
    accelerator_map[shortcut_data.chrome_command].push_back(
        ToAcceleratorMapping(shortcut_data));
  }

  // Add missing accelerators from static table.
  // e.g. IDC_CLOSE_TAB is missing because it's dynamically added.
  for (const auto& [command_id, accelerator] :
       *AcceleratorsCocoa::GetInstance()) {
    if (accelerator_map.contains(command_id) ||
        !CanConvertToAcceleratorMapping(command_id)) {
      continue;
    }

    accelerator_map[command_id].push_back(
        ToAcceleratorMapping(command_id, accelerator));
  }

  std::vector<AcceleratorMapping> result;
  for (auto& [command_id, accelerator_mappings] : accelerator_map) {
    DCHECK_NE(command_id, 0);
    for (auto& accelerator_mapping : accelerator_mappings) {
      DCHECK(accelerator_mapping.modifiers);
      result.push_back(std::move(accelerator_mapping));
    }
  }
  return result;
}

}  // namespace commands

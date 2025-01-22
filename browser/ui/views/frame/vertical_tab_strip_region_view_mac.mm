/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"

#import <Cocoa/Cocoa.h>

#include "chrome/app/chrome_command_ids.h"

static_assert(__OBJC__);
static_assert(BUILDFLAG(IS_APPLE));

namespace {

NSMenuItem* findMenuItemWithCommandRecursively(NSMenu* menu, int commandId) {
  if (NSMenuItem* menuItem = [menu itemWithTag:commandId])
    return menuItem;

  for (NSMenuItem* item in [menu itemArray]) {
    if (NSMenu* submenu = [item submenu]) {
      if (submenu == [NSApp servicesMenu])
        continue;

      if (NSMenuItem* result =
              findMenuItemWithCommandRecursively(submenu, commandId)) {
        return result;
      }
    }
  }

  return nil;
}

NSString* keyCombinationForMenuItem(NSMenuItem* item) {
  NSMutableString* string = [NSMutableString string];
  NSUInteger modifiers = item.keyEquivalentModifierMask;

  // Appended "+" after the modifier as key combination
  // is splited to each part and rendered separately.
  if (modifiers & NSEventModifierFlagCommand)
    [string appendString:@"\u2318+"];
  if (modifiers & NSEventModifierFlagControl)
    [string appendString:@"\u2303+"];
  if (modifiers & NSEventModifierFlagOption)
    [string appendString:@"\u2325+"];
  if (modifiers & NSEventModifierFlagShift)
    [string appendString:@"\u21E7+"];

  [string appendString:[item.keyEquivalent uppercaseString]];
  return string;
}

}  // namespace

std::u16string VerticalTabStripRegionView::GetShortcutTextForNewTabButton(
    BrowserView* browser_view) {
  // On Mac, users can configure accelerators for items in main menu. But
  // Chromium doesn't track them accurately.
  // https://github.com/chromium/chromium/blob/446e417e2661abe0090bfd16a2cbadd1cb88796b/chrome/browser/global_keyboard_shortcuts_mac.h#L76
  // So we should look into main menus in order to get the accurate accelerator
  // for new tab command.

  NSMenuItem* item =
      findMenuItemWithCommandRecursively([NSApp mainMenu], IDC_NEW_TAB);
  DCHECK(item);

  return base::SysNSStringToUTF16(keyCombinationForMenuItem(item));
}

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <Cocoa/Cocoa.h>

#include "base/test/run_until.h"
#include "brave/browser/ui/commands/accelerator_service.h"
#include "brave/browser/ui/commands/accelerator_service_factory.h"
#include "brave/browser/ui/commands/default_accelerators.h"
#include "brave/components/commands/common/accelerator_parsing.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest_mac.h"

namespace {

NSMenuItem* FindMenuItemWithTag(NSMenu* menu, int tag) {
  for (NSMenuItem* item in [menu itemArray]) {
    if (static_cast<int>(item.tag) == tag) {
      return item;
    }
    if (NSMenu* submenu = item.submenu;
        submenu && submenu != NSApp.servicesMenu) {
      if (NSMenuItem* found = FindMenuItemWithTag(submenu, tag)) {
        return found;
      }
    }
  }
  return nil;
}

}  // namespace

using AcceleratorMenuCoordinatorMacBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(AcceleratorMenuCoordinatorMacBrowserTest,
                       KeyEquivalentFollowsCustomizations) {
  // Make sure the coordinator observes this profile's accelerator service:
  // it reacts to browser activation.
  if (!browser()->window()->IsActive()) {
    browser()->window()->Activate();
    ASSERT_TRUE(base::test::RunUntil(
        [&]() { return browser()->window()->IsActive(); }));
  }

  auto* service = commands::AcceleratorServiceFactory::GetForContext(
      browser()->profile());
  ASSERT_TRUE(service);

  NSMenuItem* item = FindMenuItemWithTag([NSApp mainMenu], IDC_NEW_WINDOW);
  ASSERT_TRUE(item);
  NSString* default_key_equivalent = [item.keyEquivalent copy];
  const NSUInteger default_modifier_mask = item.keyEquivalentModifierMask;
  ASSERT_GT(default_key_equivalent.length, 0u);

  const auto defaults =
      service->GetDefaultAcceleratorsForCommand(IDC_NEW_WINDOW);
  ASSERT_FALSE(defaults.empty());

  // Removing the default shortcuts clears the menu item's key equivalent, so
  // the OS menu no longer dispatches them.
  for (const auto& accelerator :
       service->GetAcceleratorsForCommand(IDC_NEW_WINDOW)) {
    service->UnassignAcceleratorFromCommand(
        IDC_NEW_WINDOW, commands::ToCodesString(accelerator));
  }
  EXPECT_EQ(0u, item.keyEquivalent.length);

  // The pristine defaults are cached, so re-reading them isn't affected by
  // the mutated menu.
  auto new_defaults = commands::GetDefaultAccelerators();
  EXPECT_FALSE(new_defaults.accelerators[IDC_NEW_WINDOW].empty());

  // Resetting the command restores the menu item's key equivalent.
  service->ResetAcceleratorsForCommand(IDC_NEW_WINDOW);
  EXPECT_NSEQ(default_key_equivalent, item.keyEquivalent);
  EXPECT_EQ(default_modifier_mask, item.keyEquivalentModifierMask);

  // Reassigning the default shortcut to another command also clears the menu
  // item - the accelerator is dispatched by the browser for the new command.
  const std::string default_codes = commands::ToCodesString(defaults[0]);
  service->AssignAcceleratorToCommand(IDC_NEW_TAB, default_codes);
  EXPECT_EQ(0u, item.keyEquivalent.length);

  // And resetting the command takes it back from IDC_NEW_TAB and hands it
  // back to the menu.
  service->ResetAcceleratorsForCommand(IDC_NEW_WINDOW);
  EXPECT_NSEQ(default_key_equivalent, item.keyEquivalent);
  EXPECT_EQ(default_modifier_mask, item.keyEquivalentModifierMask);
  for (const auto& accelerator :
       service->GetAcceleratorsForCommand(IDC_NEW_TAB)) {
    EXPECT_NE(default_codes, commands::ToCodesString(accelerator));
  }
}

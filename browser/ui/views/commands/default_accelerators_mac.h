/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_COMMANDS_DEFAULT_ACCELERATORS_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_COMMANDS_DEFAULT_ACCELERATORS_MAC_H_

#include <optional>
#include <vector>

#include "base/functional/function_ref.h"
#include "chrome/browser/ui/accelerator_table.h"
#include "ui/base/accelerators/accelerator.h"

#if defined(__OBJC__)
@class NSMenu;
@class NSMenuItem;
#endif

namespace commands {

// The default accelerators on macOS, categorized by how they are dispatched.
struct MacGlobalAccelerators {
  MacGlobalAccelerators();
  ~MacGlobalAccelerators();
  MacGlobalAccelerators(MacGlobalAccelerators&&);
  MacGlobalAccelerators& operator=(MacGlobalAccelerators&&);

  // Every default accelerator, from all sources.
  std::vector<AcceleratorMapping> all;

  // Accelerators backed by a main menu NSMenuItem key equivalent. These are
  // dispatched by the OS menu, so they must not be registered with the
  // browser's FocusManager while they remain assigned to their default
  // command. AcceleratorMenuCoordinatorMac keeps the menu items in sync with
  // user customizations.
  std::vector<AcceleratorMapping> menu_backed;

  // Accelerators the user must not change: IDC_CLOSE_TAB / IDC_CLOSE_WINDOW.
  // Their key equivalents are hard-coded and dynamically swapped by upstream
  // in app_controller_mac.mm, so we can't reflect customizations for them.
  std::vector<AcceleratorMapping> unmodifiable;
};

// Returns shortcuts mapped to the global menu (which can be changed by the OS
// too, via System Settings > Keyboard > Keyboard Shortcuts > App Shortcuts —
// this is why there's no static table for these), plus global shortcuts that
// aren't present in the main menu.
//
// The result is computed once and cached: AcceleratorMenuCoordinatorMac
// mutates the live menu's key equivalents to apply user customizations, so
// re-reading the menu later (e.g. when a second profile creates its
// AcceleratorService) would treat customized values as defaults.
const MacGlobalAccelerators& GetGlobalAccelerators();

// Whether |command_id|'s key equivalents are hard-coded and dynamically
// swapped by upstream's app_controller_mac.mm, so user customizations can't
// be applied to them (IDC_CLOSE_TAB / IDC_CLOSE_WINDOW).
bool IsUnmodifiableCommand(int command_id);

#if defined(__OBJC__)
// Parses |item|'s key equivalent into an accelerator, using the same
// interpretation that GetGlobalAccelerators() uses to build the defaults.
// Returns std::nullopt if the item has no key equivalent or its tag isn't a
// known command.
std::optional<ui::Accelerator> GetAcceleratorFromMenuItem(NSMenuItem* item);

// Invokes |visitor| for every item in |menu| and its submenus, excluding the
// services menu (whose items are managed by the OS).
void ForEachMenuItem(NSMenu* menu,
                     base::FunctionRef<void(NSMenuItem*)> visitor);
#endif

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_VIEWS_COMMANDS_DEFAULT_ACCELERATORS_MAC_H_

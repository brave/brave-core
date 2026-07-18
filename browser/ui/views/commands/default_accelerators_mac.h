/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_COMMANDS_DEFAULT_ACCELERATORS_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_COMMANDS_DEFAULT_ACCELERATORS_MAC_H_

#include <optional>

#include "base/functional/function_ref.h"
#include "ui/base/accelerators/accelerator.h"

#if defined(__OBJC__)
@class NSMenu;
@class NSMenuItem;
#endif

namespace commands {

// Whether |command_id|'s key equivalents are hard-coded and dynamically
// swapped by upstream's app_controller_mac.mm, so user customizations can't
// be applied to them (i.e. IDC_CLOSE_TAB / IDC_CLOSE_WINDOW).
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

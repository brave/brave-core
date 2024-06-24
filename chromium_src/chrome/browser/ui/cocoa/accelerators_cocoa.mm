/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/app/chrome_command_ids.h"

namespace {
constexpr int kNewWindowCmdID = IDC_NEW_WINDOW;
}  // namespace

#undef IDC_NEW_WINDOW
#define IDC_NEW_WINDOW                                                        \
  IDC_NEW_OFFTHERECORD_WINDOW_TOR, ui::EF_COMMAND_DOWN | ui::EF_ALT_DOWN,     \
      ui::VKEY_N                                                              \
  }                                                                           \
  , {IDC_TOGGLE_TAB_MUTE, ui::EF_CONTROL_DOWN, ui::VKEY_M},                   \
      {IDC_COMMANDER, ui::EF_COMMAND_DOWN | ui::EF_SHIFT_DOWN, ui::VKEY_P}, { \
    kNewWindowCmdID

#include "src/chrome/browser/ui/cocoa/accelerators_cocoa.mm"

#undef IDC_NEW_WINDOW
#define IDC_NEW_WINDOW kNewWindowCmdID

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/accelerator_table.h"

#include "base/containers/extend.h"
#include "brave/app/brave_command_ids.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/ui/ui_features.h"

#define GetAcceleratorList GetAcceleratorList_ChromiumImpl
#include <chrome/browser/ui/accelerator_table.cc>
#undef GetAcceleratorList

namespace {

constexpr AcceleratorMapping kBraveAcceleratorMap[] = {
    // Ctr+Shift+S (Cmd+Shift+S on Mac)
    {ui::VKEY_S, ui::EF_PLATFORM_ACCELERATOR | ui::EF_SHIFT_DOWN,
     IDC_SHARING_HUB_SCREENSHOT},
    {ui::VKEY_M, ui::EF_CONTROL_DOWN, IDC_TOGGLE_TAB_MUTE},
    // Ctrl+B(or Cmd+B)
    {ui::VKEY_B, ui::EF_PLATFORM_ACCELERATOR, IDC_TOGGLE_SIDEBAR},
#if BUILDFLAG(IS_MAC)
    // Command-Option-N
    {ui::VKEY_N, ui::EF_ALT_DOWN | ui::EF_PLATFORM_ACCELERATOR,
     IDC_NEW_OFFTHERECORD_WINDOW_TOR},
#else
    // Alt-Shift-N
    {ui::VKEY_N, ui::EF_ALT_DOWN | ui::EF_SHIFT_DOWN,
     IDC_NEW_OFFTHERECORD_WINDOW_TOR},
#if BUILDFLAG(ENABLE_COMMANDER)
    // Open Command with Ctrl+Space
    {ui::VKEY_SPACE, ui::EF_CONTROL_DOWN, IDC_COMMANDER}
#endif
#endif
};

}  // namespace

std::vector<AcceleratorMapping> GetAcceleratorList() {
  std::vector<AcceleratorMapping> accelerator_list(
      GetAcceleratorList_ChromiumImpl());

  // Remove the upstream accelerator for new split tab on Windows and Linux, as
  // it conflicts with our existing Tor shortcut (see `kBraveAcceleratorMap`
  // above)
  if (features::IsSideBySideKeyboardShortcutEnabled()) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
    std::erase_if(accelerator_list, [](const AcceleratorMapping& m) {
      return m.keycode == ui::VKEY_N &&
             m.modifiers == (ui::EF_SHIFT_DOWN | ui::EF_ALT_DOWN) &&
             m.command_id == IDC_NEW_SPLIT_TAB;
    });
#endif
  }

  base::Extend(accelerator_list, base::span(kBraveAcceleratorMap));

  return accelerator_list;
}

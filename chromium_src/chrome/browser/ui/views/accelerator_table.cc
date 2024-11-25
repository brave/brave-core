/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/accelerator_table.h"

#include "brave/app/brave_command_ids.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "build/build_config.h"

#define GetAcceleratorList GetAcceleratorList_ChromiumImpl
#include "src/chrome/browser/ui/views/accelerator_table.cc"
#undef GetAcceleratorList

namespace {

constexpr AcceleratorMapping kBraveAcceleratorMap[] = {
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

  UNSAFE_TODO(accelerator_list.insert(
      accelerator_list.end(), kBraveAcceleratorMap,
      kBraveAcceleratorMap + std::size(kBraveAcceleratorMap)));

  return accelerator_list;
}

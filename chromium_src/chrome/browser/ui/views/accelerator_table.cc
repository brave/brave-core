/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/accelerator_table.h"

#define GetAcceleratorList GetAcceleratorList_ChromiumImpl
#include "../../../../../../chrome/browser/ui/views/accelerator_table.cc"
#undef GetAcceleratorList

namespace {

const AcceleratorMapping kBraveAcceleratorMap[] = {
#if defined(OS_MAC)
    // Command-Option-N
    {ui::VKEY_N, ui::EF_ALT_DOWN | ui::EF_PLATFORM_ACCELERATOR,
     IDC_NEW_OFFTHERECORD_WINDOW_TOR},
#else
    // Alt-Shift-N
    {ui::VKEY_N, ui::EF_ALT_DOWN | ui::EF_SHIFT_DOWN,
     IDC_NEW_OFFTHERECORD_WINDOW_TOR},
#endif
};

}  // namespace

std::vector<AcceleratorMapping> GetAcceleratorList() {
  std::vector<AcceleratorMapping> accelerator_list(
      GetAcceleratorList_ChromiumImpl());

  accelerator_list.insert(
      accelerator_list.end(),
      kBraveAcceleratorMap,
      kBraveAcceleratorMap + base::size(kBraveAcceleratorMap));

  return accelerator_list;
}

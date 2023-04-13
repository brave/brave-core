/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_COMMANDS_DEFAULT_ACCELERATORS_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_COMMANDS_DEFAULT_ACCELERATORS_MAC_H_

#include <vector>

#include "chrome/browser/ui/views/accelerator_table.h"

namespace commands {

// Returns shortcuts mapped to global menu, which can be changed dynamically
// by the OS too. This is why we don't have a static table for these.
// (System Preference > Keyboard > Keyboard Shortcuts > App shortcuts)
std::vector<AcceleratorMapping> GetGlobalAccelerators();

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_VIEWS_COMMANDS_DEFAULT_ACCELERATORS_MAC_H_

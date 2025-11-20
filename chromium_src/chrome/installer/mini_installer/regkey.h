/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_REGKEY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_REGKEY_H_

// The WriteSZValue function used to be upstream and had to be restored in Brave
// to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
#define WriteDWValue(...)    \
  WriteDWValue(__VA_ARGS__); \
  LONG WriteSZValue(const wchar_t* value_name, const wchar_t* value)

#include <chrome/installer/mini_installer/regkey.h>  // IWYU pragma: export

#undef WriteDWValue

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_REGKEY_H_

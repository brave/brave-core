/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_CONFIGURATION_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_CONFIGURATION_H_

#include <windows.h>

#define Initialize() Initialize(HMODULE module)
#define program()             \
  previous_version() const {  \
    return previous_version_; \
  }                           \
  const wchar_t* program()
#define Clear()                       \
  Clear();                            \
  void ReadResources(HMODULE module); \
  const wchar_t* previous_version_
#include <chrome/installer/mini_installer/configuration.h>  // IWYU pragma: export
#undef Clear
#undef program
#undef Initialize

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_CONFIGURATION_H_

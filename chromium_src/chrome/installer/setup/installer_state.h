/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_INSTALLER_STATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_INSTALLER_STATE_H_

#include "chrome/installer/util/util_constants.h"

#define RequiresActiveSetup                                     \
  RequiresActiveSetup() const;                                  \
  ArchiveType archive_type = ArchiveType::UNKNOWN_ARCHIVE_TYPE; \
  void Unused

#include <chrome/installer/setup/installer_state.h>  // IWYU pragma: export

#undef RequiresActiveSetup

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_INSTALLER_STATE_H_

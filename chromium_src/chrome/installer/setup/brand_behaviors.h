/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_BRAND_BEHAVIORS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_BRAND_BEHAVIORS_H_

#define UpdateInstallStatus() \
    UpdateInstallStatus(installer::ArchiveType archive_type, \
                        installer::InstallStatus install_status)

#include <chrome/installer/setup/brand_behaviors.h>  // IWYU pragma: export

#undef UpdateInstallStatus

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_BRAND_BEHAVIORS_H_

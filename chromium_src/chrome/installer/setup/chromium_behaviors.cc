/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/installer/setup/brand_behaviors.h"

#define UpdateInstallStatus() \
    UpdateInstallStatus(installer::ArchiveType archive_type, \
                        installer::InstallStatus install_status)

#include <chrome/installer/setup/chromium_behaviors.cc>

#undef UpdateInstallStatus

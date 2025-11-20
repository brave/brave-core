/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/installer/setup/brand_behaviors.h"

// UpdateInstallStatus used to have a different signature in upstream. We need
// to restore it here to support delta updates on Windows until we are on Omaha
// 4. See github.com/brave/brave-core/pull/31937.
#define UpdateInstallStatus() \
    UpdateInstallStatus(installer::ArchiveType archive_type, \
                        installer::InstallStatus install_status)

#include <chrome/installer/setup/chromium_behaviors.cc>

#undef UpdateInstallStatus

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_INSTALL_WORKER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_INSTALL_WORKER_H_

#include "src/chrome/installer/setup/install_worker.h"  // IWYU pragma: export

namespace installer {

// Method specifically used to do a one-time removal of VPN services from
// a person's machine. The service will then only be installed if they have
// purchased the Brave VPN product.
bool OneTimeVpnServiceCleanup(const base::FilePath& target_path,
                              const base::Version& new_version,
                              WorkItemList* install_list,
                              bool is_test = false);
}  // namespace installer

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_INSTALL_WORKER_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process.h"

namespace https_upgrade_exceptions {

HttpsUpgradeExceptionsService* GetHttpsUpgradeExceptionsService() {
  return g_brave_browser_process->https_upgrade_exceptions_service();
}

}  // namespace https_upgrade_exceptions

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_TLS_DEPRECATION_CONFIG_COMPONENT_INSTALLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_TLS_DEPRECATION_CONFIG_COMPONENT_INSTALLER_H_

#define ReconfigureAfterNetworkRestart            \
  ReconfigureAfterNetworkRestart_ChromiumImpl();  \
  static void ReconfigureAfterNetworkRestart
#include "../../../../../chrome/browser/component_updater/tls_deprecation_config_component_installer.h"
#undef ReconfigureAfterNetworkRestart

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_TLS_DEPRECATION_CONFIG_COMPONENT_INSTALLER_H_

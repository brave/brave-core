/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/registration.h"

#define RegisterComponentsForUpdate RegisterComponentsForUpdate_ChromiumImpl
#include "../../../../../chrome/browser/component_updater/registration.cc"  // NOLINT
#undef RegisterComponentsForUpdate

#include "chrome/browser/browser_process.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"

namespace component_updater {

void RegisterComponentsForUpdate() {
  RegisterComponentsForUpdate_ChromiumImpl();
  ComponentUpdateService* cus = g_browser_process->component_updater();
  brave_wallet::RegisterWalletDataFilesComponent(cus);
}

}  // namespace component_updater

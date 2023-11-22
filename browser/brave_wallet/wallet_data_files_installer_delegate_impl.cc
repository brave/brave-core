/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/wallet_data_files_installer_delegate_impl.h"
#include "chrome/browser/browser_process.h"

namespace brave_wallet {

component_updater::ComponentUpdateService*
WalletDataFilesInstallerDelegateImpl::GetComponentUpdater() {
  if (!g_browser_process) {  // May be null in unit tests.
    return nullptr;
  }
  return g_browser_process->component_updater();
}

}  // namespace brave_wallet

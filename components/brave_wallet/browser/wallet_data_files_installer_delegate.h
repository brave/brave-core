/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_WALLET_DATA_FILES_INSTALLER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_WALLET_DATA_FILES_INSTALLER_DELEGATE_H_

#include "components/component_updater/component_updater_service.h"

namespace brave_wallet {

class WalletDataFilesInstallerDelegate {
 public:
  WalletDataFilesInstallerDelegate() = default;
  virtual ~WalletDataFilesInstallerDelegate() = default;
  virtual component_updater::ComponentUpdateService* GetComponentUpdater() = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_WALLET_DATA_FILES_INSTALLER_DELEGATE_H_

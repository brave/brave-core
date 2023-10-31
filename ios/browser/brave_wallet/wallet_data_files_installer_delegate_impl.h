/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_WALLET_DATA_FILES_INSTALLER_DELEGATE_IMPL_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_WALLET_DATA_FILES_INSTALLER_DELEGATE_IMPL_H_

#include "brave/components/brave_wallet/browser/wallet_data_files_installer_delegate.h"

namespace brave_wallet {

class WalletDataFilesInstallerDelegateImpl
    : public WalletDataFilesInstallerDelegate {
 public:
  WalletDataFilesInstallerDelegateImpl() = default;
  ~WalletDataFilesInstallerDelegateImpl() override = default;
  component_updater::ComponentUpdateService* GetComponentUpdater() override;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_WALLET_DATA_FILES_INSTALLER_DELEGATE_IMPL_H_

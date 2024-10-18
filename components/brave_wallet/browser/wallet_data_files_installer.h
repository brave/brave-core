/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_WALLET_DATA_FILES_INSTALLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_WALLET_DATA_FILES_INSTALLER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/scoped_observation.h"
#include "base/version.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer_delegate.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client.h"

namespace base {
class FilePath;
}

class PrefService;

namespace brave_wallet {

std::optional<base::Version> GetLastInstalledWalletVersion();
void SetLastInstalledWalletVersionForTest(const base::Version& version);

class WalletDataFilesInstaller
    : public component_updater::ComponentUpdateService::Observer {
 public:
  WalletDataFilesInstaller(const WalletDataFilesInstaller&) = delete;
  WalletDataFilesInstaller& operator=(const WalletDataFilesInstaller&) = delete;

  static WalletDataFilesInstaller& GetInstance();

  void SetDelegate(std::unique_ptr<WalletDataFilesInstallerDelegate> delegate);

  void MaybeRegisterWalletDataFilesComponent(
      component_updater::ComponentUpdateService* cus,
      PrefService* local_state);

  using InstallCallback = base::OnceClosure;
  void MaybeRegisterWalletDataFilesComponentOnDemand(
      InstallCallback install_callback);

  void OnComponentReady(const base::FilePath& path);

  // component_updater::ComponentUpdateService::Observer:
  void OnEvent(const update_client::CrxUpdateItem& item) override;

  void ResetForTesting();

 private:
  friend base::NoDestructor<WalletDataFilesInstaller>;
  WalletDataFilesInstaller();
  ~WalletDataFilesInstaller() override;

  void RegisterWalletDataFilesComponentInternal(
      component_updater::ComponentUpdateService* cus);

  base::ScopedObservation<component_updater::ComponentUpdateService,
                          component_updater::ComponentUpdateService::Observer>
      component_updater_observation_{this};

  std::unique_ptr<WalletDataFilesInstallerDelegate> delegate_;
  bool registered_ = false;
  InstallCallback install_callback_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_WALLET_DATA_FILES_INSTALLER_H_

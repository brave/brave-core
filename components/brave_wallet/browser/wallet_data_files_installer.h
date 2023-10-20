/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_WALLET_DATA_FILES_INSTALLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_WALLET_DATA_FILES_INSTALLER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/version.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer_delegate.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class FilePath;
}

namespace component_updater {
class ComponentUpdateService;
}

class PrefService;

namespace brave_wallet {

absl::optional<base::Version> GetLastInstalledWalletVersion();
void SetLastInstalledWalletVersionForTest(const base::Version& version);

// TODO(jocelyn): observe for component events to clear callback when fail
class WalletDataFilesInstaller {
 public:
  WalletDataFilesInstaller(const WalletDataFilesInstaller&) = delete;
  WalletDataFilesInstaller& operator=(const WalletDataFilesInstaller&) = delete;

  static WalletDataFilesInstaller& GetInstance();

  void SetDelegate(std::unique_ptr<WalletDataFilesInstallerDelegate> delegate);

  void MaybeRegisterWalletDataFilesComponent(
      component_updater::ComponentUpdateService* cus,
      PrefService* local_state);

  // TODO(jocelyn): remove this and make Android share the same path
  //  void RegisterWalletDataFilesComponentOnDemand(
  //      component_updater::ComponentUpdateService* cus);

  using InstallCallback = base::OnceClosure;
  void MaybeRegisterWalletDataFilesComponentOnDemand(
      InstallCallback install_callback);

  void OnComponentReady(const base::FilePath& path);

 private:
  friend base::NoDestructor<WalletDataFilesInstaller>;
  WalletDataFilesInstaller();
  virtual ~WalletDataFilesInstaller();

  void RegisterWalletDataFilesComponentInternal(
      component_updater::ComponentUpdateService* cus);

  std::unique_ptr<WalletDataFilesInstallerDelegate> delegate_;
  bool registered_ = false;
  InstallCallback install_callback_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_WALLET_DATA_FILES_INSTALLER_H_

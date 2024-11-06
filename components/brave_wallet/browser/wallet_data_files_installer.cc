/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/component_updater/component_installer.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/crx_update_item.h"
#include "crypto/sha2.h"

namespace brave_wallet {

namespace {

// CRX public key hash. The extension id is: bbckkcdiepaecefgfnibemejliemjnio
// Getting the public key:
// openssl rsa -in ./wallet.pem -pubout -outform DER | openssl base64 -A >
//   wallet.pub
// openssl rsa -in ~/Desktop/wallet/wallet.pem -pubout
//   -outform DER | shasum -a 256 | head -c32 | tr 0-9a-f a-p | mvim -
const uint8_t kWalletDataFilesSha2Hash[] = {
    0x11, 0x2a, 0xa2, 0x38, 0x4f, 0x4,  0x24, 0x56, 0x5d, 0x81, 0x4c,
    0x49, 0xb8, 0x4c, 0x9d, 0x8e, 0xeb, 0xb3, 0xbd, 0x55, 0xdc, 0xf7,
    0xc0, 0x3e, 0x9b, 0x2a, 0xc2, 0xf5, 0x6a, 0x37, 0x71, 0x67};
constexpr char kWalletDataFilesDisplayName[] = "Brave Wallet data files";
constexpr char kComponentId[] = "bbckkcdiepaecefgfnibemejliemjnio";

static_assert(std::size(kWalletDataFilesSha2Hash) == crypto::kSHA256Length,
              "Wrong hash length");

std::optional<base::Version> last_installed_wallet_version;

}  // namespace

class WalletDataFilesInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  WalletDataFilesInstallerPolicy();
  ~WalletDataFilesInstallerPolicy() override = default;

 private:
  // The following methods override ComponentInstallerPolicy.
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::Value::Dict& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::Value::Dict& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      base::Value::Dict manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  bool IsBraveComponent() const override;

  WalletDataFilesInstallerPolicy(const WalletDataFilesInstallerPolicy&) =
      delete;
  WalletDataFilesInstallerPolicy operator=(
      const WalletDataFilesInstallerPolicy&) = delete;
};

WalletDataFilesInstallerPolicy::WalletDataFilesInstallerPolicy() = default;

bool WalletDataFilesInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool WalletDataFilesInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
WalletDataFilesInstallerPolicy::OnCustomInstall(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(update_client::InstallError::NONE);
}

void WalletDataFilesInstallerPolicy::OnCustomUninstall() {}

void WalletDataFilesInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::Value::Dict manifest) {
  last_installed_wallet_version = version;
  WalletDataFilesInstaller::GetInstance().OnComponentReady(path);
}

bool WalletDataFilesInstallerPolicy::VerifyInstallation(
    const base::Value::Dict& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

// The base directory on Windows looks like:
// <profile>\AppData\Local\Google\Chrome\User Data\BraveWallet\.
base::FilePath WalletDataFilesInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(kWalletBaseDirectory);
}

void WalletDataFilesInstallerPolicy::GetHash(std::vector<uint8_t>* hash) const {
  UNSAFE_TODO(hash->assign(
      kWalletDataFilesSha2Hash,
      kWalletDataFilesSha2Hash + std::size(kWalletDataFilesSha2Hash)));
}

std::string WalletDataFilesInstallerPolicy::GetName() const {
  return kWalletDataFilesDisplayName;
}

update_client::InstallerAttributes
WalletDataFilesInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool WalletDataFilesInstallerPolicy::IsBraveComponent() const {
  return true;
}

std::optional<base::Version> GetLastInstalledWalletVersion() {
  return last_installed_wallet_version;
}

void SetLastInstalledWalletVersionForTest(const base::Version& version) {
  last_installed_wallet_version = version;
}

WalletDataFilesInstaller::WalletDataFilesInstaller() = default;

WalletDataFilesInstaller::~WalletDataFilesInstaller() = default;

// static
WalletDataFilesInstaller& WalletDataFilesInstaller::GetInstance() {
  static base::NoDestructor<WalletDataFilesInstaller> instance;
  return *instance;
}

void WalletDataFilesInstaller::SetDelegate(
    std::unique_ptr<WalletDataFilesInstallerDelegate> delegate) {
  CHECK(!delegate_);

  delegate_ = std::move(delegate);
  if (auto* cus = delegate_->GetComponentUpdater()) {
    component_updater_observation_.Observe(cus);
  }
}

void WalletDataFilesInstaller::RegisterWalletDataFilesComponentInternal(
    component_updater::ComponentUpdateService* cus) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<WalletDataFilesInstallerPolicy>());

  installer->Register(
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->EnsureInstalled(kComponentId);
      }));
}

void WalletDataFilesInstaller::MaybeRegisterWalletDataFilesComponent(
    component_updater::ComponentUpdateService* cus,
    PrefService* local_state) {
  if (!IsNativeWalletEnabled() || !HasCreatedWallets(local_state)) {
    return;
  }

  registered_ = true;
  RegisterWalletDataFilesComponentInternal(cus);
}

void WalletDataFilesInstaller::MaybeRegisterWalletDataFilesComponentOnDemand(
    InstallCallback install_callback) {
  if (registered_ || !delegate_) {  // delegate_ can be nullptr in tests.
    std::move(install_callback).Run();
    return;
  }

  auto* cus = delegate_->GetComponentUpdater();
  if (!cus) {
    std::move(install_callback).Run();
    return;
  }

  registered_ = true;
  CHECK(!install_callback_);
  install_callback_ = std::move(install_callback);
  RegisterWalletDataFilesComponentInternal(cus);
}

void WalletDataFilesInstaller::OnComponentReady(const base::FilePath& path) {
  auto callback =
      install_callback_ ? std::move(install_callback_) : base::DoNothing();
  BlockchainRegistry::GetInstance()->ParseLists(path, std::move(callback));
}

void WalletDataFilesInstaller::OnEvent(
    const update_client::CrxUpdateItem& item) {
  if (item.id != kComponentId) {
    return;
  }

  if (item.state == update_client::ComponentState::kUpdateError) {
    if (install_callback_) {
      std::move(install_callback_).Run();
    }
  }
}

void WalletDataFilesInstaller::ResetForTesting() {
  component_updater_observation_.Reset();
  delegate_.reset();
  registered_ = false;
  install_callback_.Reset();
}

}  // namespace brave_wallet

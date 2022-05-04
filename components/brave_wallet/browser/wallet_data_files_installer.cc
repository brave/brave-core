/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
const char kWalletDataFilesDisplayName[] = "Brave Wallet data files";
const char kComponentId[] = "bbckkcdiepaecefgfnibemejliemjnio";

static_assert(std::size(kWalletDataFilesSha2Hash) == crypto::kSHA256Length,
              "Wrong hash length");

absl::optional<base::Version> last_installed_wallet_version;

void HandleParseTokenList(base::FilePath absolute_install_dir,
                          const std::string& filename,
                          TokenListMap* token_list_map,
                          mojom::CoinType coin_type) {
  const base::FilePath token_list_json_path =
      absolute_install_dir.AppendASCII(filename);
  std::string token_list_json;
  if (!base::ReadFileToString(token_list_json_path, &token_list_json)) {
    LOG(ERROR) << "Can't read token list file: " << filename;
  }

  if (!ParseTokenList(token_list_json, token_list_map, coin_type)) {
    LOG(ERROR) << "Can't parse token list: " << filename;
  }
}

TokenListMap TokenListReady(const base::FilePath& install_dir,
                            base::Value manifest) {
  TokenListMap lists;
  // On some platforms (e.g. Mac) we use symlinks for paths. Convert paths to
  // absolute paths to avoid unexpected failure. base::MakeAbsoluteFilePath()
  // requires IO so it can only be done in this function.
  const base::FilePath absolute_install_dir =
      base::MakeAbsoluteFilePath(install_dir);

  if (absolute_install_dir.empty()) {
    LOG(ERROR) << "Failed to get absolute install path.";
    return lists;
  }

  // Used for Ethereum mainnet
  HandleParseTokenList(absolute_install_dir, "contract-map.json", &lists,
                       mojom::CoinType::ETH);
  // Used for EVM compatabile networks including testnets
  HandleParseTokenList(absolute_install_dir, "evm-contract-map.json", &lists,
                       mojom::CoinType::ETH);
  HandleParseTokenList(absolute_install_dir, "solana-contract-map.json", &lists,
                       mojom::CoinType::SOL);

  return lists;
}

void UpdateTokenRegistry(TokenListMap lists) {
  BlockchainRegistry::GetInstance()->UpdateTokenList(std::move(lists));
}

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
      const base::Value& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::Value& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      base::Value manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;

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
    const base::Value& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(update_client::InstallError::NONE);
}

void WalletDataFilesInstallerPolicy::OnCustomUninstall() {}

void WalletDataFilesInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::Value manifest) {
  last_installed_wallet_version = version;
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(TokenListReady, path, std::move(manifest)),
      base::BindOnce(UpdateTokenRegistry));
}

bool WalletDataFilesInstallerPolicy::VerifyInstallation(
    const base::Value& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

// The base directory on Windows looks like:
// <profile>\AppData\Local\Google\Chrome\User Data\BraveWallet\.
base::FilePath WalletDataFilesInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(kWalletBaseDirectory);
}

void WalletDataFilesInstallerPolicy::GetHash(std::vector<uint8_t>* hash) const {
  hash->assign(kWalletDataFilesSha2Hash,
               kWalletDataFilesSha2Hash + std::size(kWalletDataFilesSha2Hash));
}

std::string WalletDataFilesInstallerPolicy::GetName() const {
  return kWalletDataFilesDisplayName;
}

update_client::InstallerAttributes
WalletDataFilesInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

void RegisterWalletDataFilesComponent(
    component_updater::ComponentUpdateService* cus) {
  if (brave_wallet::IsNativeWalletEnabled()) {
    auto installer =
        base::MakeRefCounted<component_updater::ComponentInstaller>(
            std::make_unique<WalletDataFilesInstallerPolicy>());
    installer->Register(
        cus, base::BindOnce([]() {
          brave_component_updater::BraveOnDemandUpdater::GetInstance()
              ->OnDemandUpdate(kComponentId);
        }));
  }
}

absl::optional<base::Version> GetLastInstalledWalletVersion() {
  return last_installed_wallet_version;
}

void SetLastInstalledWalletVersionForTest(const base::Version& version) {
  last_installed_wallet_version = version;
}

}  // namespace brave_wallet

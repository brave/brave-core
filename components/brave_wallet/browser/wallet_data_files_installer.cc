/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/json/rs/src/lib.rs.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"
#include "services/data_decoder/public/cpp/json_sanitizer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_wallet/browser/wallet_data_files_installer_android_util.h"
#endif

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

void OnSanitizedTokenList(mojom::CoinType coin,
                          data_decoder::JsonSanitizer::Result result) {
  TokenListMap lists;
  if (!result.has_value()) {
    VLOG(1) << "TokenList JSON validation error:" << result.error();
    return;
  }

  if (!ParseTokenList(*result, &lists, coin)) {
    VLOG(1) << "Can't parse token list.";
    return;
  }

  for (auto& list_pair : lists) {
    BlockchainRegistry::GetInstance()->UpdateTokenList(
        list_pair.first, std::move(list_pair.second));
  }
}

void OnSanitizedChainList(data_decoder::JsonSanitizer::Result result) {
  ChainList chains;
  if (!result.has_value()) {
    VLOG(1) << "TokenList JSON validation error:" << result.error();
    return;
  }

  if (!ParseChainList(*result, &chains)) {
    VLOG(1) << "Can't parse chain list.";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateChainList(std::move(chains));
}

void OnSanitizedDappLists(data_decoder::JsonSanitizer::Result result) {
  if (!result.has_value()) {
    VLOG(1) << "DappLists JSON validation error:" << result.error();
    return;
  }

  auto converted_json =
      std::string(json::convert_all_numbers_to_string(*result, ""));
  if (converted_json.empty()) {
    return;
  }

  absl::optional<DappListMap> lists = ParseDappLists(converted_json);
  if (!lists) {
    VLOG(1) << "Can't parse dapp lists.";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateDappList(std::move(*lists));
}

void OnSanitizedRampTokenLists(data_decoder::JsonSanitizer::Result result) {
  if (!result.has_value()) {
    VLOG(1) << "Ramp lists JSON validation error:" << result.error();
    return;
  }

  auto parsedRampTokensListMaps = ParseRampTokenListMaps(*result);
  if (!parsedRampTokensListMaps) {
    VLOG(1) << "Can't parse on/off ramp token lists.";
    return;
  }

  if (parsedRampTokensListMaps->first.empty()) {
    VLOG(1) << "On ramp supported token lists is empty.";
  } else {
    BlockchainRegistry::GetInstance()->UpdateOnRampTokenLists(
        std::move(parsedRampTokensListMaps->first));
  }

  if (parsedRampTokensListMaps->second.empty()) {
    VLOG(1) << "Off ramp supported sell token lists is empty.";
  } else {
    BlockchainRegistry::GetInstance()->UpdateOffRampTokenLists(
        std::move(parsedRampTokensListMaps->second));
  }
}

void OnSanitizedOnRampCurrenciesLists(
    data_decoder::JsonSanitizer::Result result) {
  if (!result.has_value()) {
    VLOG(1) << "OnRamp lists JSON validation error:" << result.error();
    return;
  }

  absl::optional<std::vector<mojom::OnRampCurrency>> lists =
      ParseOnRampCurrencyLists(*result);
  if (!lists) {
    VLOG(1) << "Can't parse on ramp supported sell token lists.";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateOnRampCurrenciesLists(
      std::move(*lists));
}

void OnSanitizedCoingeckoIdsMap(data_decoder::JsonSanitizer::Result result) {
  if (!result.has_value()) {
    VLOG(1) << "CoingeckoIdsMap JSON validation error:" << result.error();
    return;
  }

  absl::optional<CoingeckoIdsMap> coingecko_ids_map =
      ParseCoingeckoIdsMap(*result);
  if (!coingecko_ids_map) {
    VLOG(1) << "Can't parse coingecko-ids.json";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateCoingeckoIdsMap(
      std::move(*coingecko_ids_map));
}

void OnSanitizedOfacAddressesList(data_decoder::JsonSanitizer::Result result) {
  if (!result.has_value()) {
    VLOG(1) << "OFAC addresses list JSON validation error:" << result.error();
    return;
  }

  absl::optional<std::vector<std::string>> list =
      ParseOfacAddressesList(*result);
  if (!list) {
    VLOG(1) << "Can't parse ofac addresses list.";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateOfacAddressesList(std::move(*list));
}

void HandleJsonFileParsing(
    base::FilePath absolute_install_dir,
    const std::string& filename,
    base::OnceCallback<void(base::expected<std::string, std::string>)>
        callback) {
  const base::FilePath json_path = absolute_install_dir.AppendASCII(filename);
  std::string json_content;
  if (!base::ReadFileToString(json_path, &json_content)) {
    LOG(ERROR) << "Can't read file: " << filename;
    return;
  }

  data_decoder::JsonSanitizer::Sanitize(std::move(json_content),
                                        std::move(callback));
}

void HandleParseCoingeckoIdsMap(base::FilePath absolute_install_dir,
                                const std::string& filename) {
  HandleJsonFileParsing(absolute_install_dir, filename,
                        base::BindOnce(&OnSanitizedCoingeckoIdsMap));
}

void HandleParseTokenList(base::FilePath absolute_install_dir,
                          const std::string& filename,
                          mojom::CoinType coin_type) {
  HandleJsonFileParsing(absolute_install_dir, filename,
                        base::BindOnce(&OnSanitizedTokenList, coin_type));
}

void HandleParseChainList(base::FilePath absolute_install_dir,
                          const std::string& filename) {
  HandleJsonFileParsing(absolute_install_dir, filename,
                        base::BindOnce(&OnSanitizedChainList));
}

void HandleParseDappList(base::FilePath absolute_install_dir,
                         const std::string& filename) {
  HandleJsonFileParsing(absolute_install_dir, filename,
                        base::BindOnce(&OnSanitizedDappLists));
}

void HandleParseRampTokenLists(base::FilePath absolute_install_dir,
                               const std::string& filename) {
  HandleJsonFileParsing(absolute_install_dir, filename,
                        base::BindOnce(&OnSanitizedRampTokenLists));
}

void HandleParseOnRampCurrenciesLists(base::FilePath absolute_install_dir,
                                      const std::string& filename) {
  HandleJsonFileParsing(absolute_install_dir, filename,
                        base::BindOnce(&OnSanitizedOnRampCurrenciesLists));
}

void HandleParseOfacAddressesList(base::FilePath absolute_install_dir,
                                  const std::string& filename) {
  HandleJsonFileParsing(absolute_install_dir, filename,
                        base::BindOnce(&OnSanitizedOfacAddressesList));
}

bool ResolveAbsolutePath(const base::FilePath& input_path,
                         base::FilePath& output_path) {
  // On some platforms (e.g. Mac) we use symlinks for paths. Convert paths to
  // absolute paths to avoid unexpected failure. base::MakeAbsoluteFilePath()
  // requires IO so it can only be done in this function.
  output_path = base::MakeAbsoluteFilePath(input_path);

  if (output_path.empty()) {
    LOG(ERROR) << "Failed to get absolute install path.";
    return false;
  }

  return true;
}

void ParseCoingeckoIdsMapAndUpdateRegistry(const base::FilePath& install_dir) {
  base::FilePath absolute_install_dir;
  if (!ResolveAbsolutePath(install_dir, absolute_install_dir)) {
    return;
  }

  HandleParseCoingeckoIdsMap(absolute_install_dir, "coingecko-ids.json");
}

void ParseTokenListAndUpdateRegistry(const base::FilePath& install_dir) {
  base::FilePath absolute_install_dir;
  if (!ResolveAbsolutePath(install_dir, absolute_install_dir)) {
    return;
  }

  HandleParseTokenList(absolute_install_dir, "contract-map.json",
                       mojom::CoinType::ETH);
  HandleParseTokenList(absolute_install_dir, "evm-contract-map.json",
                       mojom::CoinType::ETH);
  HandleParseTokenList(absolute_install_dir, "solana-contract-map.json",
                       mojom::CoinType::SOL);
}

void ParseChainListAndUpdateRegistry(const base::FilePath& install_dir) {
  base::FilePath absolute_install_dir;
  if (!ResolveAbsolutePath(install_dir, absolute_install_dir)) {
    return;
  }

  HandleParseChainList(absolute_install_dir, "chainlist.json");
}

void ParseDappListsAndUpdateRegistry(const base::FilePath& install_dir) {
  base::FilePath absolute_install_dir;
  if (!ResolveAbsolutePath(install_dir, absolute_install_dir)) {
    return;
  }

  HandleParseDappList(absolute_install_dir, "dapp-lists.json");
}

void ParseOnRampListsAndUpdateRegistry(const base::FilePath& install_dir) {
  base::FilePath absolute_install_dir;
  if (!ResolveAbsolutePath(install_dir, absolute_install_dir)) {
    return;
  }

  HandleParseRampTokenLists(absolute_install_dir, "ramp-tokens.json");
  HandleParseOnRampCurrenciesLists(absolute_install_dir,
                                   "on-ramp-currency-lists.json");
}

void ParseOfacAddressesListsAndUpdateRegistry(
    const base::FilePath& install_dir) {
  base::FilePath absolute_install_dir;
  if (!ResolveAbsolutePath(install_dir, absolute_install_dir)) {
    return;
  }

  HandleParseOfacAddressesList(
      absolute_install_dir, "ofac-sanctioned-digital-currency-addresses.json");
}

}  // namespace

class WalletDataFilesInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  WalletDataFilesInstallerPolicy();
  ~WalletDataFilesInstallerPolicy() override = default;

 private:
  scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner_;
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

  WalletDataFilesInstallerPolicy(const WalletDataFilesInstallerPolicy&) =
      delete;
  WalletDataFilesInstallerPolicy operator=(
      const WalletDataFilesInstallerPolicy&) = delete;
};

WalletDataFilesInstallerPolicy::WalletDataFilesInstallerPolicy() {
  sequenced_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
}

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

  sequenced_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&ParseCoingeckoIdsMapAndUpdateRegistry, path));

  sequenced_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&ParseTokenListAndUpdateRegistry, path));

  sequenced_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&ParseChainListAndUpdateRegistry, path));

  sequenced_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&ParseDappListsAndUpdateRegistry, path));

  sequenced_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&ParseOnRampListsAndUpdateRegistry, path));

  sequenced_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&ParseOfacAddressesListsAndUpdateRegistry, path));
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
#if BUILDFLAG(IS_ANDROID)
  bool should_register_component =
      IsNativeWalletEnabled() && IsBraveWalletConfiguredOnAndroid();
#else
  bool should_register_component = IsNativeWalletEnabled();
#endif
  if (should_register_component) {
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

void RegisterWalletDataFilesComponentOnDemand(
    component_updater::ComponentUpdateService* cus) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<WalletDataFilesInstallerPolicy>());

  installer->Register(
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->OnDemandUpdate(kComponentId);
      }));
}

absl::optional<base::Version> GetLastInstalledWalletVersion() {
  return last_installed_wallet_version;
}

void SetLastInstalledWalletVersionForTest(const base::Version& version) {
  last_installed_wallet_version = version;
}

std::string GetWalletDataFilesComponentId() {
  return kComponentId;
}

}  // namespace brave_wallet

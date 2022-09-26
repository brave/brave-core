/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <unordered_set>
#include <utility>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/environment.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/field_trial_params.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/version_info/version_info.h"
#include "brave/vendor/bip39wally-core-native/include/wally_bip39.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/random.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/boringssl/src/include/openssl/evp.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {

std::string GenerateMnemonicInternal(uint8_t* entropy, size_t size) {
  char* words = nullptr;
  std::string result;
  if (bip39_mnemonic_from_bytes(nullptr, entropy, size, &words) != WALLY_OK) {
    LOG(ERROR) << __func__ << ": bip39_mnemonic_from_bytes failed";
    return result;
  }
  result = words;
  wally_free_string(words);
  return result;
}

bool IsValidEntropySize(size_t entropy_size) {
  // entropy size should be 128, 160, 192, 224, 256 bits
  if (entropy_size < 16 || entropy_size > 32 || entropy_size % 4 != 0) {
    LOG(ERROR) << __func__ << ": Entropy should be 16, 20, 24, 28, 32 bytes";
    return false;
  }
  return true;
}

std::string GetInfuraProjectID() {
  std::string project_id(BUILDFLAG(BRAVE_INFURA_PROJECT_ID));
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (env->HasVar("BRAVE_INFURA_PROJECT_ID")) {
    env->GetVar("BRAVE_INFURA_PROJECT_ID", &project_id);
  }
  return project_id;
}

const char kGanacheLocalhostURL[] = "http://localhost:7545/";
const char kSolanaLocalhostURL[] = "http://localhost:8899/";
const char kFilecoinLocalhostURL[] = "http://localhost:1234/rpc/v0";

const mojom::NetworkInfo* GetEthMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kMainnetChainId,
       "Ethereum Mainnet",
       {"https://etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ethereum",
       18,
       brave_wallet::mojom::CoinType::ETH,
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetPolygonMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kPolygonMainnetChainId,
       "Polygon Mainnet",
       {"https://polygonscan.com"},
       {},
       0,
       {},
       "MATIC",
       "MATIC",
       18,
       brave_wallet::mojom::CoinType::ETH,
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetBscMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kBinanceSmartChainMainnetChainId,
       "Binance Smart Chain Mainnet",
       {"https://bscscan.com"},
       {},
       0,
       {GURL("https://bsc-dataseed1.binance.org")},
       "BNB",
       "Binance Chain Native Token",
       18,
       brave_wallet::mojom::CoinType::ETH,
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetCeloMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kCeloMainnetChainId,
       "Celo Mainnet",
       {"https://explorer.celo.org"},
       {},
       0,
       {GURL("https://forno.celo.org")},
       "CELO",
       "CELO",
       18,
       brave_wallet::mojom::CoinType::ETH,
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetAvalancheMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kAvalancheMainnetChainId,
       "Avalanche C-Chain",
       {"https://snowtrace.io"},
       {},
       0,
       {GURL("https://api.avax.network/ext/bc/C/rpc")},
       "AVAX",
       "Avalanche",
       18,
       brave_wallet::mojom::CoinType::ETH,
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetFantomOperaMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kFantomMainnetChainId,
       "Fantom Opera",
       {"https://ftmscan.com"},
       {},
       0,
       {GURL("https://rpc.ftm.tools")},
       "FTM",
       "Fantom",
       18,
       brave_wallet::mojom::CoinType::ETH,
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetOptimismMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kOptimismMainnetChainId,
       "Optimism",
       {"https://optimistic.etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ether",
       18,
       brave_wallet::mojom::CoinType::ETH,
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetAuroraMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kAuroraMainnetChainId,
       "Aurora Mainnet",
       {"https://aurorascan.dev"},
       {},
       0,
       {},
       "ETH",
       "Ether",
       18,
       brave_wallet::mojom::CoinType::ETH,
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetRinkebyTestNetwork() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kRinkebyChainId,
       "Rinkeby Test Network",
       {"https://rinkeby.etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ethereum",
       18,
       brave_wallet::mojom::CoinType::ETH,
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetRopstenTestNetwork() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kRopstenChainId,
       "Ropsten Test Network",
       {"https://ropsten.etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ethereum",
       18,
       brave_wallet::mojom::CoinType::ETH,
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetGoerliTestNetwork() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kGoerliChainId,
       "Goerli Test Network",
       {"https://goerli.etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ethereum",
       18,
       brave_wallet::mojom::CoinType::ETH,
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetKovanTestNetwork() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kKovanChainId,
       "Kovan Test Network",
       {"https://kovan.etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ethereum",
       18,
       brave_wallet::mojom::CoinType::ETH,
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetEthLocalhost() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kLocalhostChainId,
       "Localhost",
       {kGanacheLocalhostURL},
       {},
       0,
       {GURL(kGanacheLocalhostURL)},
       "ETH",
       "Ethereum",
       18,
       brave_wallet::mojom::CoinType::ETH,
       false});
  return network_info.get();
}

// Precompiled networks available in native wallet.
const std::vector<const mojom::NetworkInfo*>& GetKnownEthNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetEthMainnet(),
      GetPolygonMainnet(),
      GetBscMainnet(),
      GetCeloMainnet(),
      GetAvalancheMainnet(),
      GetFantomOperaMainnet(),
      GetOptimismMainnet(),
      GetAuroraMainnet(),
      GetRinkebyTestNetwork(),
      GetRopstenTestNetwork(),
      GetGoerliTestNetwork(),
      GetKovanTestNetwork(),
      GetEthLocalhost(),
      // clang-format on
  });
  return *networks.get();
}

const mojom::NetworkInfo* GetSolMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kSolanaMainnet,
       "Solana Mainnet Beta",
       {"https://explorer.solana.com/"},
       {},
       0,
       {GURL("https://mainnet-beta-solana.brave.com/rpc")},
       "SOL",
       "Solana",
       9,
       brave_wallet::mojom::CoinType::SOL,
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetSolTestnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kSolanaTestnet,
       "Solana Testnet",
       {"https://explorer.solana.com/?cluster=testnet"},
       {},
       0,
       {GURL("https://api.testnet.solana.com")},
       "SOL",
       "Solana",
       9,
       brave_wallet::mojom::CoinType::SOL,
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetSolDevnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kSolanaDevnet,
       "Solana Devnet",
       {"https://explorer.solana.com/?cluster=devnet"},
       {},
       0,
       {GURL("https://api.devnet.solana.com")},
       "SOL",
       "Solana",
       9,
       brave_wallet::mojom::CoinType::SOL,
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetSolLocalhost() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kLocalhostChainId,
       "Solana Localhost",
       {"https://explorer.solana.com/"
        "?cluster=custom&customUrl=http%3A%2F%2Flocalhost%3A8899"},
       {},
       0,
       {GURL(kSolanaLocalhostURL)},
       "SOL",
       "Solana",
       9,
       brave_wallet::mojom::CoinType::SOL,
       false});
  return network_info.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownSolNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetSolMainnet(),
      GetSolTestnet(),
      GetSolDevnet(),
      GetSolLocalhost(),
      // clang-format on
  });
  return *networks.get();
}

const mojom::NetworkInfo* GetFilMainnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kFilecoinMainnet,
       "Filecoin Mainnet",
       {"https://filscan.io/tipset/message-detail"},
       {},
       0,
       {GURL("https://api.node.glif.io/rpc/v0")},
       "FIL",
       "Filecoin",
       18,
       brave_wallet::mojom::CoinType::FIL,
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilTestnet() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kFilecoinTestnet,
       "Filecoin Testnet",
       {"https://calibration.filscan.io/tipset/message-detail"},
       {},
       0,
       {GURL("https://api.calibration.node.glif.io/rpc/v0")},
       "FIL",
       "Filecoin",
       18,
       brave_wallet::mojom::CoinType::FIL,
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilLocalhost() {
  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {brave_wallet::mojom::kLocalhostChainId,
       "Filecoin Localhost",
       {kFilecoinLocalhostURL},
       {},
       0,
       {GURL(kFilecoinLocalhostURL)},
       "FIL",
       "Filecoin",
       18,
       brave_wallet::mojom::CoinType::FIL,
       false});
  return network_info.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownFilNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetFilMainnet(),
      GetFilTestnet(),
      GetFilLocalhost(),
      // clang-format on
  });
  return *networks.get();
}

const base::flat_map<std::string, std::string> kInfuraSubdomains = {
    {brave_wallet::mojom::kMainnetChainId, "mainnet"},
    {brave_wallet::mojom::kRinkebyChainId, "rinkeby"},
    {brave_wallet::mojom::kRopstenChainId, "ropsten"},
    {brave_wallet::mojom::kGoerliChainId, "goerli"},
    {brave_wallet::mojom::kKovanChainId, "kovan"}};

const base::flat_set<std::string> kInfuraChains = {
    brave_wallet::mojom::kMainnetChainId,
    brave_wallet::mojom::kPolygonMainnetChainId,
    brave_wallet::mojom::kOptimismMainnetChainId,
    brave_wallet::mojom::kAuroraMainnetChainId,
    brave_wallet::mojom::kRinkebyChainId,
    brave_wallet::mojom::kRopstenChainId,
    brave_wallet::mojom::kGoerliChainId,
    brave_wallet::mojom::kKovanChainId};

const base::flat_map<std::string, std::string> kSolanaSubdomains = {
    {brave_wallet::mojom::kSolanaMainnet, "mainnet"},
    {brave_wallet::mojom::kSolanaTestnet, "testnet"},
    {brave_wallet::mojom::kSolanaDevnet, "devnet"}};

const base::flat_map<std::string, std::string> kFilecoinSubdomains = {
    {brave_wallet::mojom::kFilecoinMainnet, "mainnet"},
    {brave_wallet::mojom::kFilecoinTestnet, "testnet"}};

// Addesses taken from https://docs.unstoppabledomains.com/developer-toolkit/
// smart-contracts/uns-smart-contracts/#proxyreader
const base::flat_map<std::string, std::string>
    kUnstoppableDomainsProxyReaderContractAddressMap = {
        {brave_wallet::mojom::kMainnetChainId,
         "0xc3C2BAB5e3e52DBF311b2aAcEf2e40344f19494E"},
        {brave_wallet::mojom::kPolygonMainnetChainId,
         "0xA3f32c8cd786dc089Bd1fC175F2707223aeE5d00"}};

constexpr const char kEnsRegistryContractAddress[] =
    "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e";

const base::Value::List* GetCustomNetworksList(PrefService* prefs,
                                               mojom::CoinType coin) {
  const base::Value* custom_networks =
      prefs->GetDictionary(kBraveWalletCustomNetworks);
  if (!custom_networks)
    return nullptr;
  return custom_networks->GetDict().FindList(GetPrefKeyForCoinType(coin));
}

std::vector<mojom::NetworkInfoPtr> MergeKnownAndCustomChains(
    std::vector<mojom::NetworkInfoPtr> known_chains,
    std::vector<mojom::NetworkInfoPtr> custom_chains) {
  std::vector<mojom::NetworkInfoPtr> result;
  // Put all known chains to result, replace with custom one if chain_id
  // matches.
  for (auto& known_chain : known_chains) {
    bool add_known_chain = true;
    for (auto& custom_chain : custom_chains) {
      if (custom_chain && custom_chain->chain_id == known_chain->chain_id) {
        result.push_back(std::move(custom_chain));
        add_known_chain = false;
        break;
      }
    }
    if (add_known_chain) {
      result.push_back(std::move(known_chain));
    }
  }

  // Put all remaining custom chains to result.
  for (auto& custom_chain : custom_chains) {
    if (custom_chain)
      result.push_back(std::move(custom_chain));
  }

  return result;
}

}  // namespace

GURL AddInfuraProjectId(const GURL& url) {
  DCHECK(url.is_valid()) << url.possibly_invalid_spec();
  GURL::Replacements replacements;
  auto path = GetInfuraProjectID();
  replacements.SetPathStr(path);
  return url.ReplaceComponents(replacements);
}

GURL MaybeAddInfuraProjectId(const GURL& url) {
  if (!url.is_valid())
    return GURL();
  for (const auto& infura_chain_id : kInfuraChains) {
    if (GetInfuraURLForKnownChainId(infura_chain_id) == url) {
      return AddInfuraProjectId(url);
    }
  }
  return url;
}

mojom::NetworkInfoPtr GetKnownChain(PrefService* prefs,
                                    const std::string& chain_id,
                                    mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    for (const auto* network : GetKnownEthNetworks()) {
      if (network->chain_id != chain_id)
        continue;

      auto result = network->Clone();
      if (result->rpc_endpoints.empty()) {
        result->active_rpc_endpoint_index = 0;
        result->rpc_endpoints = {GURL(GetInfuraURLForKnownChainId(chain_id))};
      }

      if (prefs && chain_id == brave_wallet::mojom::kLocalhostChainId) {
        result->is_eip1559 = prefs->GetBoolean(kSupportEip1559OnLocalhostChain);
      }

      return result;
    }
    return nullptr;
  }
  if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks()) {
      if (network->chain_id == chain_id)
        return network->Clone();
    }
    return nullptr;
  }
  if (coin == mojom::CoinType::SOL) {
    for (const auto* network : GetKnownSolNetworks()) {
      if (network->chain_id == chain_id)
        return network->Clone();
    }
    return nullptr;
  }
  NOTREACHED();
  return nullptr;
}

mojom::NetworkInfoPtr GetCustomChain(PrefService* prefs,
                                     const std::string& chain_id,
                                     mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs, coin);
  if (!custom_list)
    return nullptr;
  for (const auto& it : *custom_list) {
    if (auto opt_chain_id =
            brave_wallet::ExtractChainIdFromValue(it.GetIfDict())) {
      if (chain_id == *opt_chain_id)
        return brave_wallet::ValueToNetworkInfo(it);
    }
  }
  return nullptr;
}

mojom::NetworkInfoPtr GetChain(PrefService* prefs,
                               const std::string& chain_id,
                               mojom::CoinType coin) {
  if (auto custom_chain = GetCustomChain(prefs, chain_id, coin)) {
    return custom_chain;
  }
  if (auto known_chain = GetKnownChain(prefs, chain_id, coin)) {
    return known_chain;
  }

  return nullptr;
}

GURL GetInfuraURLForKnownChainId(const std::string& chain_id) {
  auto endpoint = brave_wallet::GetInfuraEndpointForKnownChainId(chain_id);
  if (!endpoint.empty())
    return GURL(endpoint);

  auto subdomain = brave_wallet::GetInfuraSubdomainForKnownChainId(chain_id);
  if (subdomain.empty())
    return GURL();
  return GURL(
      base::StringPrintf("https://%s-infura.brave.com/", subdomain.c_str()));
}

std::string GetInfuraEndpointForKnownChainId(const std::string& chain_id) {
  const auto& endpoints = GetInfuraChainEndpoints();
  if (endpoints.contains(chain_id))
    return endpoints.at(chain_id);
  return std::string();
}

std::string GetInfuraSubdomainForKnownChainId(const std::string& chain_id) {
  if (kInfuraSubdomains.contains(chain_id))
    return kInfuraSubdomains.at(chain_id);
  return std::string();
}

std::string GetSolanaSubdomainForKnownChainId(const std::string& chain_id) {
  if (kSolanaSubdomains.contains(chain_id))
    return kSolanaSubdomains.at(chain_id);
  return std::string();
}

std::string GetFilecoinSubdomainForKnownChainId(const std::string& chain_id) {
  if (kFilecoinSubdomains.contains(chain_id))
    return kFilecoinSubdomains.at(chain_id);
  return std::string();
}

std::vector<mojom::NetworkInfoPtr> GetAllCustomChains(PrefService* prefs,
                                                      mojom::CoinType coin) {
  std::vector<mojom::NetworkInfoPtr> result;
  auto* custom_list = GetCustomNetworksList(prefs, coin);
  if (!custom_list)
    return result;

  for (const auto& it : *custom_list) {
    mojom::NetworkInfoPtr chain = ValueToNetworkInfo(it);
    if (chain) {
      DCHECK_EQ(chain->coin, coin);
      result.push_back(std::move(chain));
    }
  }

  return result;
}

bool KnownChainExists(const std::string& chain_id, mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    for (const auto* network : GetKnownEthNetworks()) {
      if (network->chain_id == chain_id)
        return true;
    }
  } else if (coin == mojom::CoinType::SOL) {
    for (const auto* network : GetKnownSolNetworks())
      if (network->chain_id == chain_id)
        return true;
  } else if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks())
      if (network->chain_id == chain_id)
        return true;
  } else {
    NOTREACHED() << coin;
  }
  return false;
}

bool CustomChainExists(PrefService* prefs,
                       const std::string& custom_chain_id,
                       mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs, coin);
  if (!custom_list)
    return false;
  for (const auto& it : *custom_list) {
    if (auto chain_id = ExtractChainIdFromValue(it.GetIfDict())) {
      if (*chain_id == custom_chain_id)
        return true;
    }
  }
  return false;
}

bool IsNativeWalletEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kNativeBraveWalletFeature);
}

bool IsFilecoinEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kBraveWalletFilecoinFeature);
}

bool IsDappsSupportEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kBraveWalletDappsSupportFeature);
}

bool IsSolanaEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kBraveWalletSolanaFeature);
}

bool ShouldCreateDefaultSolanaAccount() {
  return IsSolanaEnabled() &&
         brave_wallet::features::kCreateDefaultSolanaAccount.Get();
}

std::vector<brave_wallet::mojom::NetworkInfoPtr>
GetAllKnownNetworksForTesting() {
  std::vector<brave_wallet::mojom::NetworkInfoPtr> result;
  for (const auto* network : GetKnownEthNetworks())
    result.push_back(network->Clone());
  return result;
}

std::string GenerateMnemonic(size_t entropy_size) {
  if (!IsValidEntropySize(entropy_size))
    return "";

  std::vector<uint8_t> entropy(entropy_size);
  crypto::RandBytes(&entropy[0], entropy.size());

  return GenerateMnemonicInternal(entropy.data(), entropy.size());
}

std::string GenerateMnemonicForTest(const std::vector<uint8_t>& entropy) {
  return GenerateMnemonicInternal(const_cast<uint8_t*>(entropy.data()),
                                  entropy.size());
}

std::unique_ptr<std::vector<uint8_t>> MnemonicToSeed(
    const std::string& mnemonic,
    const std::string& passphrase) {
  if (!IsValidMnemonic(mnemonic))
    return nullptr;

  std::unique_ptr<std::vector<uint8_t>> seed =
      std::make_unique<std::vector<uint8_t>>(64);
  const std::string salt = "mnemonic" + passphrase;
  int rv = PKCS5_PBKDF2_HMAC(mnemonic.data(), mnemonic.length(),
                             reinterpret_cast<const uint8_t*>(salt.data()),
                             salt.length(), 2048, EVP_sha512(), seed->size(),
                             seed->data());
  return rv == 1 ? std::move(seed) : nullptr;
}

std::unique_ptr<std::vector<uint8_t>> MnemonicToEntropy(
    const std::string& mnemonic) {
  if (!IsValidMnemonic(mnemonic))
    return nullptr;

  const std::vector<std::string> words = SplitString(
      mnemonic, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  // size in bytes
  size_t entropy_size = 0;
  switch (words.size()) {
    case 12:
      entropy_size = 16;
      break;
    case 15:
      entropy_size = 20;
      break;
    case 18:
      entropy_size = 24;
      break;
    case 21:
      entropy_size = 28;
      break;
    case 24:
      entropy_size = 32;
      break;
    default:
      NOTREACHED();
  }
  DCHECK(IsValidEntropySize(entropy_size)) << entropy_size;

  std::unique_ptr<std::vector<uint8_t>> entropy =
      std::make_unique<std::vector<uint8_t>>(entropy_size);

  size_t written;
  if (bip39_mnemonic_to_bytes(nullptr, mnemonic.c_str(), entropy->data(),
                              entropy->size(), &written) != WALLY_OK) {
    LOG(ERROR) << "bip39_mnemonic_to_bytes failed";
    return nullptr;
  }
  return entropy;
}

bool IsValidMnemonic(const std::string& mnemonic) {
  if (bip39_mnemonic_validate(nullptr, mnemonic.c_str()) != WALLY_OK) {
    LOG(ERROR) << __func__ << ": Invalid mnemonic: " << mnemonic;
    return false;
  }
  return true;
}

bool EncodeString(const std::string& input, std::string* output) {
  if (!base::IsStringUTF8(input))
    return false;

  if (input.empty()) {
    *output =
        "0x0000000000000000000000000000000000000000000000000000000000000000";
    return true;
  }

  // Encode count for this string
  bool success =
      PadHexEncodedParameter(Uint256ValueToHex(input.size()), output);
  if (!success)
    return false;

  // Encode string.
  *output += base::ToLowerASCII(base::HexEncode(input.data(), input.size()));

  // Pad 0 to right.
  size_t last_row_len = input.size() % 32;
  if (last_row_len == 0) {
    return true;
  }

  size_t padding_len = (32 - last_row_len) * 2;
  *output += std::string(padding_len, '0');
  return true;
}

bool EncodeStringArray(const std::vector<std::string>& input,
                       std::string* output) {
  // Write count of elements.
  bool success = PadHexEncodedParameter(
      Uint256ValueToHex(static_cast<uint256_t>(input.size())), output);
  if (!success)
    return false;

  // Write offsets to array elements.
  size_t data_offset = input.size() * 32;  // Offset to first element.
  std::string encoded_offset;
  success =
      PadHexEncodedParameter(Uint256ValueToHex(data_offset), &encoded_offset);
  if (!success)
    return false;
  *output += encoded_offset.substr(2);

  for (size_t i = 1; i < input.size(); i++) {
    // Offset for ith element =
    //     offset for i-1th + 32 * (count for i-1th) +
    //     32 * ceil(i-1th.size() / 32.0) (length of encoding for i-1th).
    std::string encoded_offset_for_element;
    size_t rows = std::ceil(input[i - 1].size() / 32.0);
    data_offset += (rows + 1) * 32;

    success = PadHexEncodedParameter(Uint256ValueToHex(data_offset),
                                     &encoded_offset_for_element);
    if (!success)
      return false;
    *output += encoded_offset_for_element.substr(2);
  }

  // Write count and encoding for array elements.
  for (const auto& entry : input) {
    std::string encoded_string;
    success = EncodeString(entry, &encoded_string);
    if (!success)
      return false;
    *output += encoded_string.substr(2);
  }

  return true;
}

bool DecodeString(size_t offset,
                  const std::string& input,
                  std::string* output) {
  if (!output->empty())
    return false;

  // Decode count.
  uint256_t count = 0;
  size_t len = 64;
  if (offset + len > input.size() ||
      !HexValueToUint256("0x" + input.substr(offset, len), &count)) {
    return false;
  }

  // Empty string case.
  if (!count) {
    *output = "";
    return true;
  }

  // Decode string.
  offset += len;
  len = static_cast<size_t>(count) * 2;
  return offset + len <= input.size() &&
         base::HexStringToString(input.substr(offset, len), output);
}

bool DecodeStringArray(const std::string& input,
                       std::vector<std::string>* output) {
  // Get count of array.
  uint256_t count = 0;
  if (!HexValueToUint256("0x" + input.substr(0, 64), &count) ||
      input.size() == 0) {
    return false;
  }

  // Decode count and string for each array element.
  *output = std::vector<std::string>(static_cast<size_t>(count), "");
  size_t offset = 64;  // Offset to count of first element.
  for (size_t i = 0; i < static_cast<size_t>(count); i++) {
    // Get the starting data offset for each string element.
    uint256_t data_offset;
    if (offset + 64 > input.size() ||
        !HexValueToUint256("0x" + input.substr(offset, 64), &data_offset)) {
      return false;
    }

    // Decode each string.
    size_t string_offset =
        64 /* count */ + static_cast<size_t>(data_offset) * 2;
    if (string_offset > input.size() ||
        !DecodeString(string_offset, input, &output->at(i))) {
      return false;
    }

    offset += 64;  // Offset for next count.
  }

  return true;
}

// Updates preferences for when the wallet is unlocked.
// This is done in a utils function instead of in the KeyringService
// because we call it both from the old extension and the new wallet when
// it unlocks.
void UpdateLastUnlockPref(PrefService* prefs) {
  prefs->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
}

base::Value::Dict TransactionReceiptToValue(
    const TransactionReceipt& tx_receipt) {
  base::Value::Dict dict;
  dict.Set("transaction_hash", tx_receipt.transaction_hash);
  dict.Set("transaction_index",
           Uint256ValueToHex(tx_receipt.transaction_index));
  dict.Set("block_hash", tx_receipt.block_hash);
  dict.Set("block_number", Uint256ValueToHex(tx_receipt.block_number));
  dict.Set("from", tx_receipt.from);
  dict.Set("to", tx_receipt.to);
  dict.Set("cumulative_gas_used",
           Uint256ValueToHex(tx_receipt.cumulative_gas_used));
  dict.Set("gas_used", Uint256ValueToHex(tx_receipt.gas_used));
  dict.Set("contract_address", tx_receipt.contract_address);
  // TODO(darkdh): logs
  dict.Set("logs_bloom", tx_receipt.logs_bloom);
  dict.Set("status", tx_receipt.status);
  return dict;
}

absl::optional<TransactionReceipt> ValueToTransactionReceipt(
    const base::Value::Dict& value) {
  TransactionReceipt tx_receipt;
  const std::string* transaction_hash = value.FindString("transaction_hash");
  if (!transaction_hash)
    return absl::nullopt;
  tx_receipt.transaction_hash = *transaction_hash;

  const std::string* transaction_index = value.FindString("transaction_index");
  if (!transaction_index)
    return absl::nullopt;
  uint256_t transaction_index_uint;
  if (!HexValueToUint256(*transaction_index, &transaction_index_uint))
    return absl::nullopt;
  tx_receipt.transaction_index = transaction_index_uint;

  const std::string* block_hash = value.FindString("block_hash");
  if (!block_hash)
    return absl::nullopt;
  tx_receipt.block_hash = *block_hash;

  const std::string* block_number = value.FindString("block_number");
  if (!block_number)
    return absl::nullopt;
  uint256_t block_number_uint;
  if (!HexValueToUint256(*block_number, &block_number_uint))
    return absl::nullopt;
  tx_receipt.block_number = block_number_uint;

  const std::string* from = value.FindString("from");
  if (!from)
    return absl::nullopt;
  tx_receipt.from = *from;

  const std::string* to = value.FindString("to");
  if (!to)
    return absl::nullopt;
  tx_receipt.to = *to;

  const std::string* cumulative_gas_used =
      value.FindString("cumulative_gas_used");
  if (!cumulative_gas_used)
    return absl::nullopt;
  uint256_t cumulative_gas_used_uint;
  if (!HexValueToUint256(*cumulative_gas_used, &cumulative_gas_used_uint))
    return absl::nullopt;
  tx_receipt.cumulative_gas_used = cumulative_gas_used_uint;

  const std::string* gas_used = value.FindString("gas_used");
  if (!gas_used)
    return absl::nullopt;
  uint256_t gas_used_uint;
  if (!HexValueToUint256(*gas_used, &gas_used_uint))
    return absl::nullopt;
  tx_receipt.gas_used = gas_used_uint;

  const std::string* contract_address = value.FindString("contract_address");
  if (!contract_address)
    return absl::nullopt;
  tx_receipt.contract_address = *contract_address;

  // TODO(darkdh): logs
  const std::string* logs_bloom = value.FindString("logs_bloom");
  if (!logs_bloom)
    return absl::nullopt;
  tx_receipt.logs_bloom = *logs_bloom;

  absl::optional<bool> status = value.FindBool("status");
  if (!status)
    return absl::nullopt;
  tx_receipt.status = *status;

  return tx_receipt;
}

std::vector<mojom::NetworkInfoPtr> GetAllKnownChains(PrefService* prefs,
                                                     mojom::CoinType coin) {
  std::vector<mojom::NetworkInfoPtr> result;

  if (coin == mojom::CoinType::ETH) {
    for (const auto* network : GetKnownEthNetworks()) {
      // TODO(apaymyshev): GetKnownEthChain also loops over
      // GetKnownEthNetworks().
      result.push_back(
          GetKnownChain(prefs, network->chain_id, mojom::CoinType::ETH));
    }
    return result;
  }

  if (coin == mojom::CoinType::SOL) {
    for (const auto* network : GetKnownSolNetworks())
      result.push_back(network->Clone());
    return result;
  }

  if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks())
      result.push_back(network->Clone());
    return result;
  }

  NOTREACHED();
  return result;
}

GURL GetNetworkURL(PrefService* prefs,
                   const std::string& chain_id,
                   mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    if (auto custom_chain =
            GetCustomChain(prefs, chain_id, mojom::CoinType::ETH)) {
      return MaybeAddInfuraProjectId(GetActiveEndpointUrl(*custom_chain));
    } else if (auto known_chain =
                   GetKnownChain(prefs, chain_id, mojom::CoinType::ETH)) {
      return MaybeAddInfuraProjectId(GetActiveEndpointUrl(*known_chain));
    }
  } else if (coin == mojom::CoinType::SOL) {
    for (const auto* network : GetKnownSolNetworks()) {
      if (network->chain_id == chain_id) {
        return GetActiveEndpointUrl(*network);
      }
    }
  } else if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks()) {
      if (network->chain_id == chain_id) {
        return GetActiveEndpointUrl(*network);
      }
    }
  }
  return GURL();
}

std::vector<mojom::NetworkInfoPtr> GetAllChains(PrefService* prefs,
                                                mojom::CoinType coin) {
  return MergeKnownAndCustomChains(GetAllKnownChains(prefs, coin),
                                   GetAllCustomChains(prefs, coin));
}

std::vector<std::string> GetAllKnownSolNetworkIds() {
  std::vector<std::string> network_ids;
  for (const auto* network : GetKnownSolNetworks()) {
    std::string network_id = GetKnownSolNetworkId(network->chain_id);
    if (!network_id.empty())
      network_ids.push_back(network_id);
  }
  return network_ids;
}

std::vector<std::string> GetAllKnownFilNetworkIds() {
  std::vector<std::string> network_ids;
  for (const auto* network : GetKnownFilNetworks()) {
    std::string network_id = GetKnownFilNetworkId(network->chain_id);
    if (!network_id.empty())
      network_ids.push_back(network_id);
  }
  return network_ids;
}

std::vector<std::string> GetAllKnownEthNetworkIds() {
  std::vector<std::string> network_ids;
  for (const auto* network : GetKnownEthNetworks()) {
    std::string network_id = GetKnownEthNetworkId(network->chain_id);
    if (!network_id.empty())
      network_ids.push_back(network_id);
  }
  return network_ids;
}

std::string GetKnownEthNetworkId(const std::string& chain_id) {
  auto subdomain = GetInfuraSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty())
    return subdomain;

  // For known networks not in kInfuraSubdomains:
  //   localhost: Use the first RPC URL.
  //   other: Use chain ID like other custom networks.
  for (const auto* network : GetKnownEthNetworks()) {
    if (network->chain_id != chain_id)
      continue;
    if (chain_id == mojom::kLocalhostChainId)
      return network->rpc_endpoints.front().spec();
    return chain_id;
  }

  return "";
}

std::string GetKnownSolNetworkId(const std::string& chain_id) {
  auto subdomain = GetSolanaSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty())
    return subdomain;

  // Separate check for localhost in known networks as it is predefined but
  // does not have predefined subdomain.
  if (chain_id == mojom::kLocalhostChainId) {
    for (const auto* network : GetKnownSolNetworks()) {
      if (network->chain_id == chain_id) {
        return network->rpc_endpoints.front().spec();
      }
    }
  }

  return "";
}

std::string GetKnownFilNetworkId(const std::string& chain_id) {
  auto subdomain = GetFilecoinSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty())
    return subdomain;

  // Separate check for localhost in known networks as it is predefined but
  // does not have predefined subdomain.
  if (chain_id == mojom::kLocalhostChainId) {
    for (const auto* network : GetKnownFilNetworks()) {
      if (network->chain_id == chain_id) {
        return network->rpc_endpoints.front().spec();
      }
    }
  }

  return "";
}

std::string GetKnownNetworkId(mojom::CoinType coin,
                              const std::string& chain_id) {
  if (coin == mojom::CoinType::ETH)
    return GetKnownEthNetworkId(chain_id);
  if (coin == mojom::CoinType::SOL)
    return GetKnownSolNetworkId(chain_id);
  if (coin == mojom::CoinType::FIL)
    return GetKnownFilNetworkId(chain_id);
  return "";
}

std::string GetNetworkId(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id) {
  if (chain_id.empty())
    return "";

  std::string id = GetKnownNetworkId(coin, chain_id);
  if (!id.empty())
    return id;

  DCHECK(prefs);
  if (coin == mojom::CoinType::ETH) {
    for (const auto& network :
         GetAllCustomChains(prefs, mojom::CoinType::ETH)) {
      if (network->chain_id != chain_id)
        continue;
      id = chain_id;
      break;
    }
  }

  return id;
}

mojom::DefaultWallet GetDefaultEthereumWallet(PrefService* prefs) {
  return static_cast<brave_wallet::mojom::DefaultWallet>(
      prefs->GetInteger(kDefaultEthereumWallet));
}

mojom::DefaultWallet GetDefaultSolanaWallet(PrefService* prefs) {
  return static_cast<brave_wallet::mojom::DefaultWallet>(
      prefs->GetInteger(kDefaultSolanaWallet));
}

void SetDefaultEthereumWallet(PrefService* prefs,
                              mojom::DefaultWallet default_wallet) {
  // We should not be using this value anymore
  DCHECK(default_wallet != mojom::DefaultWallet::AskDeprecated);
  prefs->SetInteger(kDefaultEthereumWallet, static_cast<int>(default_wallet));
}

void SetDefaultSolanaWallet(PrefService* prefs,
                            mojom::DefaultWallet default_wallet) {
  // We should not be using these values anymore
  DCHECK(default_wallet != mojom::DefaultWallet::AskDeprecated);
  DCHECK(default_wallet != mojom::DefaultWallet::CryptoWallets);
  prefs->SetInteger(kDefaultSolanaWallet, static_cast<int>(default_wallet));
}

void SetDefaultBaseCurrency(PrefService* prefs, const std::string& currency) {
  prefs->SetString(kDefaultBaseCurrency, currency);
}

std::string GetDefaultBaseCurrency(PrefService* prefs) {
  return prefs->GetString(kDefaultBaseCurrency);
}

void SetDefaultBaseCryptocurrency(PrefService* prefs,
                                  const std::string& cryptocurrency) {
  prefs->SetString(kDefaultBaseCryptocurrency, cryptocurrency);
}

bool GetShowWalletTestNetworks(PrefService* prefs) {
  return prefs->GetBoolean(kShowWalletTestNetworks);
}

mojom::CoinType GetSelectedCoin(PrefService* prefs) {
  return static_cast<brave_wallet::mojom::CoinType>(
      prefs->GetInteger(kBraveWalletSelectedCoin));
}

void SetSelectedCoin(PrefService* prefs, mojom::CoinType coin) {
  prefs->SetInteger(kBraveWalletSelectedCoin, static_cast<int>(coin));
}

std::string GetDefaultBaseCryptocurrency(PrefService* prefs) {
  return prefs->GetString(kDefaultBaseCryptocurrency);
}

GURL GetUnstoppableDomainsRpcUrl(const std::string& chain_id) {
  if (chain_id == brave_wallet::mojom::kMainnetChainId ||
      chain_id == brave_wallet::mojom::kPolygonMainnetChainId) {
    return AddInfuraProjectId(GURL(GetInfuraURLForKnownChainId(chain_id)));
  }

  NOTREACHED();
  return GURL();
}

std::string GetUnstoppableDomainsProxyReaderContractAddress(
    const std::string& chain_id) {
  if (kUnstoppableDomainsProxyReaderContractAddressMap.contains(chain_id))
    return kUnstoppableDomainsProxyReaderContractAddressMap.at(chain_id);
  return "";
}

std::string GetEnsRegistryContractAddress(const std::string& chain_id) {
  DCHECK_EQ(chain_id, brave_wallet::mojom::kMainnetChainId);
  return kEnsRegistryContractAddress;
}

void AddCustomNetwork(PrefService* prefs, const mojom::NetworkInfo& chain) {
  DCHECK(prefs);
  // FIL and SOL allow custom chains only over known ones.
  DCHECK(chain.coin == mojom::CoinType::ETH ||
         KnownChainExists(chain.chain_id, chain.coin));

  {  // Update needs to be done before GetNetworkId below.
    DictionaryPrefUpdate update(prefs, kBraveWalletCustomNetworks);
    base::Value::Dict& dict = update.Get()->GetDict();
    // TODO(cdesouza): Once cr106 is merged, this FindList should be replaced
    // with EnsureList.
    base::Value::List* list = dict.FindList(GetPrefKeyForCoinType(chain.coin));
    if (!list) {
      list = dict.Set(GetPrefKeyForCoinType(chain.coin), base::Value::List())
                 ->GetIfList();
    }
    CHECK(list);
    list->Append(NetworkInfoToValue(chain));
  }

  if (chain.coin != mojom::CoinType::ETH)
    return;

  const std::string network_id =
      GetNetworkId(prefs, mojom::CoinType::ETH, chain.chain_id);
  DCHECK(!network_id.empty());  // Not possible for a custom network.

  DictionaryPrefUpdate update(prefs, kBraveWalletUserAssets);
  base::Value::Dict& user_assets_pref = update.Get()->GetDict();
  base::Value::List& asset_list =
      user_assets_pref
          .SetByDottedPath(base::StrCat({GetPrefKeyForCoinType(chain.coin), ".",
                                         network_id}),
                           base::Value::List())
          ->GetList();

  base::Value::Dict native_asset;
  native_asset.Set("address", "");
  native_asset.Set("name", chain.symbol_name);
  native_asset.Set("symbol", chain.symbol);
  native_asset.Set("is_erc20", false);
  native_asset.Set("is_erc721", false);
  native_asset.Set("decimals", chain.decimals);
  native_asset.Set("visible", true);
  native_asset.Set("logo", chain.icon_urls.empty() ? "" : chain.icon_urls[0]);

  asset_list.Append(std::move(native_asset));
}

void RemoveCustomNetwork(PrefService* prefs,
                         const std::string& chain_id_to_remove,
                         mojom::CoinType coin) {
  DCHECK(prefs);

  DictionaryPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value::Dict& dict = update.Get()->GetDict();
  base::Value::List* list = dict.FindList(GetPrefKeyForCoinType(coin));
  if (!list)
    return;
  list->EraseIf([&chain_id_to_remove](const base::Value& v) {
    DCHECK(v.is_dict());
    auto* chain_id_value = v.GetDict().FindString("chainId");
    if (!chain_id_value)
      return false;
    return *chain_id_value == chain_id_to_remove;
  });
}

std::vector<std::string> GetAllHiddenNetworks(PrefService* prefs,
                                              mojom::CoinType coin) {
  std::vector<std::string> result;
  const base::Value::Dict& hidden_networks =
      prefs->GetValueDict(kBraveWalletHiddenNetworks);

  auto* hidden_eth_networks =
      hidden_networks.FindList(GetPrefKeyForCoinType(coin));
  if (!hidden_eth_networks)
    return result;

  for (const auto& it : *hidden_eth_networks) {
    auto* chain_id = it.GetIfString();
    if (chain_id)
      result.push_back(std::move(*chain_id));
  }

  return result;
}

void AddHiddenNetwork(PrefService* prefs,
                      mojom::CoinType coin,
                      const std::string& chain_id) {
  DictionaryPrefUpdate update(prefs, kBraveWalletHiddenNetworks);
  base::Value::Dict& dict = update.Get()->GetDict();
  // TODO(cdesouza): Once cr106 is merged, this FindList should be replaced with
  // EnsureList.
  base::Value::List* list = dict.FindList(GetPrefKeyForCoinType(coin));
  if (!list) {
    list =
        dict.Set(GetPrefKeyForCoinType(coin), base::Value::List())->GetIfList();
  }
  CHECK(list);
  if (!base::Contains(*list, base::Value(chain_id))) {
    list->Append(chain_id);
  }
}

void RemoveHiddenNetwork(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id) {
  DictionaryPrefUpdate update(prefs, kBraveWalletHiddenNetworks);
  base::Value::Dict& dict = update.Get()->GetDict();
  base::Value::List* list = dict.FindList(GetPrefKeyForCoinType(coin));
  if (!list)
    return;
  list->EraseIf([&](const base::Value& v) {
    auto* chain_id_string = v.GetIfString();
    if (!chain_id_string)
      return false;
    return *chain_id_string == chain_id;
  });
}

std::string GetCurrentChainId(PrefService* prefs, mojom::CoinType coin) {
  const base::Value::Dict& selected_networks =
      prefs->GetValueDict(kBraveWalletSelectedNetworks);
  const std::string* chain_id =
      selected_networks.FindString(GetPrefKeyForCoinType(coin));
  if (!chain_id)
    return std::string();

  return *chain_id;
}

std::string GetPrefKeyForCoinType(mojom::CoinType coin) {
  switch (coin) {
    case mojom::CoinType::ETH:
      return kEthereumPrefKey;
    case mojom::CoinType::FIL:
      return kFilecoinPrefKey;
    case mojom::CoinType::SOL:
      return kSolanaPrefKey;
  }
  NOTREACHED();
  return "";
}

std::string eTLDPlusOne(const url::Origin& origin) {
  return net::registry_controlled_domains::GetDomainAndRegistry(
      origin, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

mojom::OriginInfoPtr MakeOriginInfo(const url::Origin& origin) {
  return mojom::OriginInfo::New(origin, origin.Serialize(),
                                eTLDPlusOne(origin));
}

// Returns a string used for web3_clientVersion in the form of
// Brave/v[version]
std::string GetWeb3ClientVersion() {
  return base::StringPrintf(
      "BraveWallet/v%s", version_info::GetBraveChromiumVersionNumber().c_str());
}

bool IsFilecoinKeyringId(const std::string& keyring_id) {
  return keyring_id == mojom::kFilecoinKeyringId ||
         keyring_id == mojom::kFilecoinTestnetKeyringId;
}

std::string GetFilecoinKeyringId(const std::string& network) {
  if (network == mojom::kFilecoinMainnet) {
    return mojom::kFilecoinKeyringId;
  } else if (network == mojom::kFilecoinTestnet ||
             network == mojom::kLocalhostChainId) {
    return mojom::kFilecoinTestnetKeyringId;
  }
  NOTREACHED() << "Unsupported chain id for filecoin " << network;
  return mojom::kFilecoinMainnet;
}

std::string GetFilecoinChainId(const std::string& keyring_id) {
  if (keyring_id == mojom::kFilecoinKeyringId) {
    return mojom::kFilecoinMainnet;
  } else if (keyring_id == mojom::kFilecoinTestnetKeyringId) {
    return mojom::kFilecoinTestnet;
  }
  NOTREACHED() << "Unsupported keyring id for filecoin";
  return "";
}

mojom::CoinType GetCoinForKeyring(const std::string& keyring_id) {
  if (IsFilecoinKeyringId(keyring_id)) {
    return mojom::CoinType::FIL;
  } else if (keyring_id == mojom::kSolanaKeyringId) {
    return mojom::CoinType::SOL;
  }

  DCHECK_EQ(keyring_id, mojom::kDefaultKeyringId);
  return mojom::CoinType::ETH;
}

GURL GetActiveEndpointUrl(const mojom::NetworkInfo& chain) {
  if (chain.active_rpc_endpoint_index >= 0 &&
      static_cast<size_t>(chain.active_rpc_endpoint_index) <
          chain.rpc_endpoints.size()) {
    return chain.rpc_endpoints[chain.active_rpc_endpoint_index];
  }
  return GURL();
}

}  // namespace brave_wallet

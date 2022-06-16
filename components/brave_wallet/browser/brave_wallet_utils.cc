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

// Precompiled networks available in native wallet.
const brave_wallet::mojom::NetworkInfo kKnownEthNetworks[] = {
    {brave_wallet::mojom::kMainnetChainId,
     "Ethereum Mainnet",
     {"https://etherscan.io"},
     {},
     {},
     "ETH",
     "Ethereum",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kPolygonMainnetChainId,
     "Polygon Mainnet",
     {"https://polygonscan.com"},
     {},
     {},
     "MATIC",
     "MATIC",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kBinanceSmartChainMainnetChainId,
     "Binance Smart Chain Mainnet",
     {"https://bscscan.com"},
     {},
     {"https://bsc-dataseed1.binance.org"},
     "BNB",
     "Binance Chain Native Token",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kCeloMainnetChainId,
     "Celo Mainnet",
     {"https://explorer.celo.org"},
     {},
     {"https://forno.celo.org"},
     "CELO",
     "CELO",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kAvalancheMainnetChainId,
     "Avalanche C-Chain",
     {"https://snowtrace.io"},
     {},
     {"https://api.avax.network/ext/bc/C/rpc"},
     "AVAX",
     "Avalanche",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kFantomMainnetChainId,
     "Fantom Opera",
     {"https://ftmscan.com"},
     {},
     {"https://rpc.ftm.tools"},
     "FTM",
     "Fantom",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kOptimismMainnetChainId,
     "Optimism",
     {"https://optimistic.etherscan.io"},
     {},
     {"https://mainnet.optimism.io"},
     "ETH",
     "Ether",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kRinkebyChainId,
     "Rinkeby Test Network",
     {"https://rinkeby.etherscan.io"},
     {},
     {},
     "ETH",
     "Ethereum",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kRopstenChainId,
     "Ropsten Test Network",
     {"https://ropsten.etherscan.io"},
     {},
     {},
     "ETH",
     "Ethereum",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kGoerliChainId,
     "Goerli Test Network",
     {"https://goerli.etherscan.io"},
     {},
     {},
     "ETH",
     "Ethereum",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kKovanChainId,
     "Kovan Test Network",
     {"https://kovan.etherscan.io"},
     {},
     {},
     "ETH",
     "Ethereum",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(mojom::NetworkInfoDataETH::New(true))},
    {brave_wallet::mojom::kLocalhostChainId,
     "Localhost",
     {kGanacheLocalhostURL},
     {},
     {kGanacheLocalhostURL},
     "ETH",
     "Ethereum",
     18,
     brave_wallet::mojom::CoinType::ETH,
     mojom::NetworkInfoData::NewEthData(
         mojom::NetworkInfoDataETH::New(false))}};

const brave_wallet::mojom::NetworkInfo kKnownSolNetworks[] = {
    {brave_wallet::mojom::kSolanaMainnet,
     "Solana Mainnet Beta",
     {"https://explorer.solana.com/"},
     {},
     {"https://mainnet-beta-solana.brave.com/rpc"},
     "SOL",
     "Solana",
     9,
     brave_wallet::mojom::CoinType::SOL,
     nullptr},
    {brave_wallet::mojom::kSolanaTestnet,
     "Solana Testnet",
     {"https://explorer.solana.com/?cluster=testnet"},
     {},
     {"https://testnet-solana.brave.com/rpc"},
     "SOL",
     "Solana",
     9,
     brave_wallet::mojom::CoinType::SOL,
     nullptr},
    {brave_wallet::mojom::kSolanaDevnet,
     "Solana Devnet",
     {"https://explorer.solana.com/?cluster=devnet"},
     {},
     {"https://api.devnet.solana.com"},
     "SOL",
     "Solana",
     9,
     brave_wallet::mojom::CoinType::SOL,
     nullptr},
    {brave_wallet::mojom::kLocalhostChainId,
     "Solana Localhost",
     {"https://explorer.solana.com/"
      "?cluster=custom&customUrl=http%3A%2F%2Flocalhost%3A8899"},
     {},
     {kSolanaLocalhostURL},
     "SOL",
     "Solana",
     9,
     brave_wallet::mojom::CoinType::SOL,
     nullptr}};

const brave_wallet::mojom::NetworkInfo kKnownFilNetworks[] = {
    {brave_wallet::mojom::kFilecoinMainnet,
     "Filecoin Mainnet",
     {"https://filscan.io/tipset/message-detail"},
     {},
     {"https://api.node.glif.io/rpc/v0"},
     "FIL",
     "Filecoin",
     18,
     brave_wallet::mojom::CoinType::FIL,
     nullptr}};

const brave_wallet::mojom::NetworkInfo kKnownFilNetworksWithTestnet[] = {
    {brave_wallet::mojom::kFilecoinMainnet,
     "Filecoin Mainnet",
     {"https://filscan.io/tipset/message-detail"},
     {},
     {"https://api.node.glif.io/rpc/v0"},
     "FIL",
     "Filecoin",
     18,
     brave_wallet::mojom::CoinType::FIL,
     nullptr},
    {brave_wallet::mojom::kFilecoinTestnet,
     "Filecoin Testnet",
     {"https://calibration.filscan.io/tipset/message-detail"},
     {},
     {"https://calibration.node.glif.io/rpc/v0"},
     "FIL",
     "Filecoin",
     18,
     brave_wallet::mojom::CoinType::FIL,
     nullptr},
    {brave_wallet::mojom::kLocalhostChainId,
     "Filecoin Localhost",
     {kFilecoinLocalhostURL},
     {},
     {kFilecoinLocalhostURL},
     "FIL",
     "Filecoin",
     18,
     brave_wallet::mojom::CoinType::FIL,
     nullptr}};

const std::vector<brave_wallet::mojom::NetworkInfoPtr>&
GetActualFilNetworksInfo() {
  static const base::NoDestructor<
      std::vector<brave_wallet::mojom::NetworkInfoPtr>>
      networks_info([] {
        std::vector<brave_wallet::mojom::NetworkInfoPtr> networks_info;
        if (IsFilecoinTestnetEnabled()) {
          for (const auto& a : kKnownFilNetworksWithTestnet) {
            networks_info.push_back(a.Clone());
          }
        } else {
          for (const auto& a : kKnownFilNetworks) {
            networks_info.push_back(a.Clone());
          }
        }
        return networks_info;
      }());

  return *networks_info;
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

std::string GetInfuraURLForKnownChainId(const std::string& chain_id) {
  if (chain_id == brave_wallet::mojom::kPolygonMainnetChainId) {
    return kPolygonMainnetEndpoint;
  }

  auto subdomain = brave_wallet::GetInfuraSubdomainForKnownChainId(chain_id);
  if (subdomain.empty())
    return std::string();
  return base::StringPrintf("https://%s-infura.brave.com/", subdomain.c_str());
}

const base::Value::List* GetEthCustomNetworksList(PrefService* prefs) {
  const base::Value* custom_networks =
      prefs->GetDictionary(kBraveWalletCustomNetworks);
  if (!custom_networks)
    return nullptr;
  return custom_networks->GetDict().FindList(kEthereumPrefKey);
}

std::vector<mojom::NetworkInfoPtr> MergeEthChains(
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
  DCHECK(url.is_valid()) << url.possibly_invalid_spec();
  for (const auto& infura_chain_id : kInfuraChains) {
    if (GetInfuraURLForKnownChainId(infura_chain_id) == url) {
      return AddInfuraProjectId(url);
    }
  }
  return url;
}

mojom::NetworkInfoPtr GetKnownEthChain(PrefService* prefs,
                                       const std::string& chain_id) {
  for (const auto& network : kKnownEthNetworks) {
    if (network.chain_id != chain_id)
      continue;

    auto result = network.Clone();
    if (result->rpc_urls.empty())
      result->rpc_urls.push_back(GetInfuraURLForKnownChainId(chain_id));

    if (prefs && chain_id == brave_wallet::mojom::kLocalhostChainId) {
      result->data->set_eth_data(mojom::NetworkInfoDataETH::New(
          prefs->GetBoolean(kSupportEip1559OnLocalhostChain)));
    }

    return result;
  }
  return nullptr;
}

mojom::NetworkInfoPtr GetCustomEthChain(PrefService* prefs,
                                        const std::string& chain_id) {
  const base::Value::List* custom_list = GetEthCustomNetworksList(prefs);
  if (!custom_list)
    return nullptr;
  for (const auto& it : *custom_list) {
    if (auto opt_chain_id =
            brave_wallet::ExtractChainIdFromValue(it.GetIfDict())) {
      if (chain_id == *opt_chain_id)
        return brave_wallet::ValueToEthNetworkInfo(it);
    }
  }
  return nullptr;
}

mojom::NetworkInfoPtr GetChain(PrefService* prefs,
                               const std::string& chain_id,
                               mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    if (auto custom_chain = GetCustomEthChain(prefs, chain_id)) {
      return custom_chain;
    }
    if (auto known_chain = GetKnownEthChain(prefs, chain_id)) {
      return known_chain;
    }
  } else if (coin == mojom::CoinType::SOL) {
    for (const auto& network : kKnownSolNetworks) {
      if (network.chain_id == chain_id) {
        return network.Clone();
      }
    }
  } else if (coin == mojom::CoinType::FIL) {
    for (const auto& network : GetActualFilNetworksInfo()) {
      if (network->chain_id == chain_id) {
        return network.Clone();
      }
    }
  }

  return nullptr;
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

std::vector<mojom::NetworkInfoPtr> GetAllEthCustomChains(PrefService* prefs) {
  std::vector<mojom::NetworkInfoPtr> result;
  auto* custom_list = GetEthCustomNetworksList(prefs);
  if (!custom_list)
    return result;

  for (const auto& it : *custom_list) {
    mojom::NetworkInfoPtr chain = brave_wallet::ValueToEthNetworkInfo(it);
    if (chain)
      result.push_back(std::move(chain));
  }

  return result;
}

bool KnownEthChainExists(const std::string& chain_id) {
  for (const auto& network : kKnownEthNetworks) {
    if (network.chain_id == chain_id)
      return true;
  }
  return false;
}

bool CustomEthChainExists(PrefService* prefs,
                          const std::string& custom_chain_id) {
  const base::Value::List* custom_list = GetEthCustomNetworksList(prefs);
  if (!custom_list)
    return false;
  for (const auto& it : *custom_list) {
    if (auto chain_id = brave_wallet::ExtractChainIdFromValue(it.GetIfDict())) {
      if (*chain_id == custom_chain_id)
        return true;
    }
  }
  return false;
}

GURL GetFirstValidChainURL(const std::vector<std::string>& chain_urls) {
  if (chain_urls.empty())
    return GURL();
  for (const std::string& spec : chain_urls) {
    GURL url(spec);
    if (url.is_valid() && url.SchemeIsHTTPOrHTTPS() &&
        !base::Contains(spec, "${INFURA_API_KEY}") &&
        !base::Contains(spec, "${ALCHEMY_API_KEY}") &&
        !base::Contains(spec, "${API_KEY}") &&
        !base::Contains(spec, "${PULSECHAIN_API_KEY}")) {
      return url;
    }
  }
  GURL front_url(chain_urls.front());
  if (front_url.is_valid())
    return front_url;
  return GURL();
}

bool IsNativeWalletEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kNativeBraveWalletFeature);
}

bool IsFilecoinEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kBraveWalletFilecoinFeature);
}

// This is needed only for unit tests, not to be used in prod.
bool IsFilecoinTestnetEnabled() {
  return base::FeatureList::IsEnabled(
             brave_wallet::features::kBraveWalletFilecoinFeature) &&
         brave_wallet::features::kFilecoinTestnetEnabled.Get();
}

bool IsDappsSupportEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kBraveWalletDappsSupportFeature);
}

bool IsSolanaEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kBraveWalletSolanaFeature);
}

std::vector<brave_wallet::mojom::NetworkInfoPtr>
GetAllKnownNetworksForTesting() {
  std::vector<brave_wallet::mojom::NetworkInfoPtr> result;
  for (const auto& network : kKnownEthNetworks)
    result.push_back(network.Clone());
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
  for (size_t i = 0; i < input.size(); i++) {
    std::string encoded_string;
    success = EncodeString(input[i], &encoded_string);
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

base::Value TransactionReceiptToValue(const TransactionReceipt& tx_receipt) {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("transaction_hash", tx_receipt.transaction_hash);
  dict.SetStringKey("transaction_index",
                    Uint256ValueToHex(tx_receipt.transaction_index));
  dict.SetStringKey("block_hash", tx_receipt.block_hash);
  dict.SetStringKey("block_number", Uint256ValueToHex(tx_receipt.block_number));
  dict.SetStringKey("from", tx_receipt.from);
  dict.SetStringKey("to", tx_receipt.to);
  dict.SetStringKey("cumulative_gas_used",
                    Uint256ValueToHex(tx_receipt.cumulative_gas_used));
  dict.SetStringKey("gas_used", Uint256ValueToHex(tx_receipt.gas_used));
  dict.SetStringKey("contract_address", tx_receipt.contract_address);
  // TODO(darkdh): logs
  dict.SetStringKey("logs_bloom", tx_receipt.logs_bloom);
  dict.SetBoolKey("status", tx_receipt.status);
  return dict;
}

absl::optional<TransactionReceipt> ValueToTransactionReceipt(
    const base::Value& value) {
  TransactionReceipt tx_receipt;
  const std::string* transaction_hash = value.FindStringKey("transaction_hash");
  if (!transaction_hash)
    return absl::nullopt;
  tx_receipt.transaction_hash = *transaction_hash;

  const std::string* transaction_index =
      value.FindStringKey("transaction_index");
  if (!transaction_index)
    return absl::nullopt;
  uint256_t transaction_index_uint;
  if (!HexValueToUint256(*transaction_index, &transaction_index_uint))
    return absl::nullopt;
  tx_receipt.transaction_index = transaction_index_uint;

  const std::string* block_hash = value.FindStringKey("block_hash");
  if (!block_hash)
    return absl::nullopt;
  tx_receipt.block_hash = *block_hash;

  const std::string* block_number = value.FindStringKey("block_number");
  if (!block_number)
    return absl::nullopt;
  uint256_t block_number_uint;
  if (!HexValueToUint256(*block_number, &block_number_uint))
    return absl::nullopt;
  tx_receipt.block_number = block_number_uint;

  const std::string* from = value.FindStringKey("from");
  if (!from)
    return absl::nullopt;
  tx_receipt.from = *from;

  const std::string* to = value.FindStringKey("to");
  if (!to)
    return absl::nullopt;
  tx_receipt.to = *to;

  const std::string* cumulative_gas_used =
      value.FindStringKey("cumulative_gas_used");
  if (!cumulative_gas_used)
    return absl::nullopt;
  uint256_t cumulative_gas_used_uint;
  if (!HexValueToUint256(*cumulative_gas_used, &cumulative_gas_used_uint))
    return absl::nullopt;
  tx_receipt.cumulative_gas_used = cumulative_gas_used_uint;

  const std::string* gas_used = value.FindStringKey("gas_used");
  if (!gas_used)
    return absl::nullopt;
  uint256_t gas_used_uint;
  if (!HexValueToUint256(*gas_used, &gas_used_uint))
    return absl::nullopt;
  tx_receipt.gas_used = gas_used_uint;

  const std::string* contract_address = value.FindStringKey("contract_address");
  if (!contract_address)
    return absl::nullopt;
  tx_receipt.contract_address = *contract_address;

  // TODO(darkdh): logs
  const std::string* logs_bloom = value.FindStringKey("logs_bloom");
  if (!logs_bloom)
    return absl::nullopt;
  tx_receipt.logs_bloom = *logs_bloom;

  absl::optional<bool> status = value.FindBoolKey("status");
  if (!status)
    return absl::nullopt;
  tx_receipt.status = *status;

  return tx_receipt;
}

std::vector<mojom::NetworkInfoPtr> GetAllKnownEthChains(PrefService* prefs) {
  std::vector<mojom::NetworkInfoPtr> chains;
  for (const auto& network : kKnownEthNetworks) {
    // TODO(apaymyshev): GetKnownEthChain also loops over kKnownEthNetworks.
    chains.push_back(GetKnownEthChain(prefs, network.chain_id));
  }
  return chains;
}

GURL GetNetworkURL(PrefService* prefs,
                   const std::string& chain_id,
                   mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    if (auto custom_chain = GetCustomEthChain(prefs, chain_id)) {
      return MaybeAddInfuraProjectId(
          GetFirstValidChainURL(custom_chain->rpc_urls));
    } else if (auto known_chain = GetKnownEthChain(prefs, chain_id)) {
      return MaybeAddInfuraProjectId(GURL(known_chain->rpc_urls.front()));
    }
  } else if (coin == mojom::CoinType::SOL) {
    for (const auto& network : kKnownSolNetworks) {
      if (network.chain_id == chain_id && network.rpc_urls.size()) {
        return GURL(network.rpc_urls.front());
      }
    }
  } else if (coin == mojom::CoinType::FIL) {
    for (const auto& network : GetActualFilNetworksInfo()) {
      if (network->chain_id == chain_id && network->rpc_urls.size()) {
        return GURL(network->rpc_urls.front());
      }
    }
  }
  return GURL();
}

std::vector<mojom::NetworkInfoPtr> GetAllChains(PrefService* prefs,
                                                mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    return MergeEthChains(GetAllKnownEthChains(prefs),
                          GetAllEthCustomChains(prefs));
  } else if (coin == mojom::CoinType::SOL) {
    return GetAllKnownSolChains();
  } else if (coin == mojom::CoinType::FIL) {
    return GetAllKnownFilChains();
  }
  NOTREACHED();
  return {};
}

std::vector<mojom::NetworkInfoPtr> GetAllKnownFilChains() {
  std::vector<mojom::NetworkInfoPtr> result;
  for (const auto& network : GetActualFilNetworksInfo())
    result.push_back(network.Clone());
  return result;
}

std::vector<mojom::NetworkInfoPtr> GetAllKnownSolChains() {
  std::vector<mojom::NetworkInfoPtr> result;
  for (const auto& network : kKnownSolNetworks)
    result.push_back(network.Clone());
  return result;
}

std::vector<std::string> GetAllKnownSolNetworkIds() {
  std::vector<std::string> network_ids;
  for (const auto& network : kKnownSolNetworks) {
    std::string network_id = GetKnownSolNetworkId(network.chain_id);
    if (!network_id.empty())
      network_ids.push_back(network_id);
  }
  return network_ids;
}

std::vector<std::string> GetAllKnownFilNetworkIds() {
  std::vector<std::string> network_ids;
  for (const auto& network : GetActualFilNetworksInfo()) {
    std::string network_id = GetKnownFilNetworkId(network->chain_id);
    if (!network_id.empty())
      network_ids.push_back(network_id);
  }
  return network_ids;
}

std::vector<std::string> GetAllKnownEthNetworkIds() {
  std::vector<std::string> network_ids;
  for (const auto& network : kKnownEthNetworks) {
    std::string network_id = GetKnownEthNetworkId(network.chain_id);
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
  for (const auto& network : kKnownEthNetworks) {
    if (network.chain_id != chain_id)
      continue;
    if (chain_id == mojom::kLocalhostChainId)
      return GURL(network.rpc_urls.front()).spec();
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
    for (const auto& network : kKnownSolNetworks) {
      if (network.chain_id == chain_id) {
        return GURL(network.rpc_urls.front()).spec();
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
    for (const auto& network : GetActualFilNetworksInfo()) {
      if (network->chain_id == chain_id) {
        return GURL(network->rpc_urls.front()).spec();
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
    for (const auto& network : GetAllEthCustomChains(prefs)) {
      if (network->chain_id != chain_id)
        continue;
      id = chain_id;
      break;
    }
  }

  return id;
}

mojom::DefaultWallet GetDefaultWallet(PrefService* prefs) {
  return static_cast<brave_wallet::mojom::DefaultWallet>(
      prefs->GetInteger(kDefaultWallet2));
}

void SetDefaultWallet(PrefService* prefs, mojom::DefaultWallet default_wallet) {
  // We should not be using this value anymore
  DCHECK(default_wallet != mojom::DefaultWallet::AskDeprecated);
  prefs->SetInteger(kDefaultWallet2, static_cast<int>(default_wallet));
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

  base::Value value = brave_wallet::EthNetworkInfoToValue(chain);

  {  // Update needs to be done before GetNetworkId below.
    DictionaryPrefUpdate update(prefs, kBraveWalletCustomNetworks);
    base::Value* dict = update.Get();
    CHECK(dict);
    base::Value* list = dict->FindKey(kEthereumPrefKey);
    if (!list) {
      list =
          dict->SetKey(kEthereumPrefKey, base::Value(base::Value::Type::LIST));
    }
    CHECK(list);
    list->Append(std::move(value));
  }

  const std::string network_id =
      GetNetworkId(prefs, mojom::CoinType::ETH, chain.chain_id);
  DCHECK(!network_id.empty());  // Not possible for a custom network.

  DictionaryPrefUpdate update(prefs, kBraveWalletUserAssets);
  base::Value* user_assets_pref = update.Get();
  base::Value* asset_list = user_assets_pref->SetPath(
      base::StrCat({kEthereumPrefKey, ".", network_id}),
      base::Value(base::Value::Type::LIST));

  base::Value native_asset(base::Value::Type::DICTIONARY);
  native_asset.SetStringKey("address", "");
  native_asset.SetStringKey("name", chain.symbol_name);
  native_asset.SetStringKey("symbol", chain.symbol);
  native_asset.SetBoolKey("is_erc20", false);
  native_asset.SetBoolKey("is_erc721", false);
  native_asset.SetIntKey("decimals", chain.decimals);
  native_asset.SetBoolKey("visible", true);
  native_asset.SetStringKey("logo",
                            chain.icon_urls.empty() ? "" : chain.icon_urls[0]);

  asset_list->Append(std::move(native_asset));
}

void RemoveCustomNetwork(PrefService* prefs,
                         const std::string& chain_id_to_remove) {
  DCHECK(prefs);

  DictionaryPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value* dict = update.Get();
  CHECK(dict);
  base::Value* list = dict->FindKey(kEthereumPrefKey);
  if (!list)
    return;
  list->EraseListValueIf([&](const base::Value& v) {
    auto* chain_id_value = v.FindStringKey("chainId");
    if (!chain_id_value)
      return false;
    return *chain_id_value == chain_id_to_remove;
  });
}

std::vector<std::string> GetAllHiddenNetworks(PrefService* prefs,
                                              mojom::CoinType coin) {
  if (coin != mojom::CoinType::ETH) {
    NOTREACHED();
    return {};
  }

  std::vector<std::string> result;
  const base::Value* hidden_networks =
      prefs->GetDictionary(kBraveWalletHiddenNetworks);
  if (!hidden_networks)
    return result;

  auto* hidden_eth_networks =
      hidden_networks->GetDict().FindList(kEthereumPrefKey);
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
  if (coin != mojom::CoinType::ETH) {
    NOTREACHED();
    return;
  }

  DictionaryPrefUpdate update(prefs, kBraveWalletHiddenNetworks);
  base::Value* dict = update.Get();
  CHECK(dict);
  base::Value* list = dict->FindKey(kEthereumPrefKey);
  if (!list) {
    list = dict->SetKey(kEthereumPrefKey, base::Value(base::Value::Type::LIST));
  }
  CHECK(list);
  if (!base::Contains(list->GetList(), base::Value(chain_id))) {
    list->Append(base::Value(chain_id));
  }
}

void RemoveHiddenNetwork(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id) {
  if (coin != mojom::CoinType::ETH) {
    NOTREACHED();
    return;
  }

  DictionaryPrefUpdate update(prefs, kBraveWalletHiddenNetworks);
  base::Value* dict = update.Get();
  CHECK(dict);
  base::Value* list = dict->FindKey(kEthereumPrefKey);
  if (!list)
    return;
  list->EraseListValueIf([&](const base::Value& v) {
    auto* chain_id_string = v.GetIfString();
    if (!chain_id_string)
      return false;
    return *chain_id_string == chain_id;
  });
}

std::string GetCurrentChainId(PrefService* prefs, mojom::CoinType coin) {
  const base::Value* selected_networks =
      prefs->GetDictionary(kBraveWalletSelectedNetworks);
  DCHECK(selected_networks);
  const std::string* chain_id =
      selected_networks->FindStringKey(GetPrefKeyForCoinType(coin));
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

}  // namespace brave_wallet

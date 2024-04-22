/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/environment.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/version_info/version_info.h"
#include "brave/third_party/bip39wally-core-native/include/wally_bip39.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/random.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/boringssl/src/include/openssl/evp.h"
#include "ui/base/l10n/l10n_util.h"
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
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Ethereum Mainnet",
       {"https://etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ethereum",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetPolygonMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kPolygonMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Polygon Mainnet",
       {"https://polygonscan.com"},
       {},
       0,
       {},
       "MATIC",
       "MATIC",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetBscMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kBinanceSmartChainMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Binance Smart Chain Mainnet",
       {"https://bscscan.com"},
       {},
       0,
       {GURL("https://bsc-dataseed1.binance.org")},
       "BNB",
       "Binance Chain Native Token",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetAvalancheMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kAvalancheMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Avalanche C-Chain",
       {"https://snowtrace.io"},
       {},
       0,
       {},
       "AVAX",
       "Avalanche",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetOptimismMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kOptimismMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Optimism",
       {"https://optimistic.etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ether",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetAuroraMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kAuroraMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Aurora Mainnet",
       {"https://aurorascan.dev"},
       {},
       0,
       {},
       "ETH",
       "Ether",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetNeonEVMMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kNeonEVMMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Neon EVM",
       {"https://neonscan.org"},
       {},
       0,
       {GURL("https://neon-proxy-mainnet.solana.p2p.org")},
       "NEON",
       "Neon",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetGoerliTestNetwork() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kGoerliChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Goerli Test Network",
       {"https://goerli.etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ethereum",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetSepoliaTestNetwork() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kSepoliaChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Sepolia Test Network",
       {"https://sepolia.etherscan.io"},
       {},
       0,
       {},
       "ETH",
       "Ethereum",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetEthLocalhost() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kLocalhostChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Localhost",
       {kGanacheLocalhostURL},
       {},
       0,
       {GURL(kGanacheLocalhostURL)},
       "ETH",
       "Ethereum",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilecoinEthereumMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kFilecoinEthereumMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin EVM Mainnet",
       {"https://filfox.info/en/message"},
       {},
       0,
       {GURL("https://api.node.glif.io/rpc/v1")},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       true});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilecoinEthereumTestnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kFilecoinEthereumTestnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin EVM Testnet",
       {"https://calibration.filfox.info/en/message"},
       {},
       0,
       {GURL("https://api.calibration.node.glif.io/rpc/v1")},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       true});
  return network_info.get();
}

// Precompiled networks available in native wallet.
const std::vector<const mojom::NetworkInfo*>& GetKnownEthNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetEthMainnet(),
      GetAuroraMainnet(),
      GetPolygonMainnet(),
      GetBscMainnet(),
      GetOptimismMainnet(),
      GetAvalancheMainnet(),
      GetFilecoinEthereumMainnet(),
      GetNeonEVMMainnet(),
      GetGoerliTestNetwork(),
      GetSepoliaTestNetwork(),
      GetFilecoinEthereumTestnet(),
      GetEthLocalhost()
      // clang-format on
  });
  return *networks.get();
}

const mojom::NetworkInfo* GetSolMainnet() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kSolanaMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Solana Mainnet Beta",
       {"https://explorer.solana.com/"},
       {},
       0,
       {GURL("https://mainnet-beta-solana.brave.com/rpc")},
       "SOL",
       "Solana",
       9,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetSolTestnet() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kSolanaTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Solana Testnet",
       {"https://explorer.solana.com/?cluster=testnet"},
       {},
       0,
       {GURL("https://api.testnet.solana.com")},
       "SOL",
       "Solana",
       9,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetSolDevnet() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kSolanaDevnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Solana Devnet",
       {"https://explorer.solana.com/?cluster=devnet"},
       {},
       0,
       {GURL("https://api.devnet.solana.com")},
       "SOL",
       "Solana",
       9,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetSolLocalhost() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kLocalhostChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Solana Localhost",
       {"https://explorer.solana.com/"
        "?cluster=custom&customUrl=http%3A%2F%2Flocalhost%3A8899"},
       {},
       0,
       {GURL(kSolanaLocalhostURL)},
       "SOL",
       "Solana",
       9,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
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
  const auto coin = mojom::CoinType::FIL;
  const auto* chain_id = mojom::kFilecoinMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin Mainnet",
       {"https://filscan.io/tipset/message-detail"},
       {},
       0,
       {GURL("https://api.node.glif.io/rpc/v0")},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilTestnet() {
  const auto coin = mojom::CoinType::FIL;
  const auto* chain_id = mojom::kFilecoinTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin Testnet",
       {"https://calibration.filscan.io/tipset/message-detail"},
       {},
       0,
       {GURL("https://api.calibration.node.glif.io/rpc/v0")},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilLocalhost() {
  const auto coin = mojom::CoinType::FIL;
  const auto* chain_id = mojom::kLocalhostChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin Localhost",
       {kFilecoinLocalhostURL},
       {},
       0,
       {GURL(kFilecoinLocalhostURL)},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
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

GURL ZCashMainnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kZCashMainnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GURL("https://mainnet.lightwalletd.com:9067/");
}

GURL ZCashTestnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kZCashTestnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GURL("https://testnet.lightwalletd.com:9067/");
}

const mojom::NetworkInfo* GetBitcoinMainnet() {
  const auto coin = mojom::CoinType::BTC;
  const auto* chain_id = mojom::kBitcoinMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Bitcoin Mainnet",
       {"https://www.blockchain.com/explorer"},
       {},
       0,
       {GURL(kBitcoinMainnetRpcEndpoint)},
       "BTC",
       "Bitcoin",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetBitcoinTestnet() {
  const auto coin = mojom::CoinType::BTC;
  const auto* chain_id = mojom::kBitcoinTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Bitcoin Testnet",
       {"https://blockstream.info/testnet"},
       {},
       0,
       {GURL(kBitcoinTestnetRpcEndpoint)},
       "BTC",
       "Bitcoin",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetZCashMainnet() {
  const auto coin = mojom::CoinType::ZEC;
  const auto* chain_id = mojom::kZCashMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Zcash Mainnet",
       {"https://zcashblockexplorer.com/transactions"},
       {},
       0,
       {ZCashMainnetRpcUrl()},
       "ZEC",
       "Zcash",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const mojom::NetworkInfo* GetZCashTestnet() {
  const auto coin = mojom::CoinType::ZEC;
  const auto* chain_id = mojom::kZCashTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Zcash Testnet",
       {"https://testnet.zcashblockexplorer.com/transactions"},
       {},
       0,
       {ZCashTestnetRpcUrl()},
       "ZEC",
       "Zcash",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id),
       false});
  return network_info.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownZCashNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetZCashMainnet(),
      GetZCashTestnet(),
      // clang-format on
  });
  return *networks.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownBitcoinNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetBitcoinMainnet(),
      GetBitcoinTestnet(),
      // clang-format on
  });
  return *networks.get();
}

const base::flat_map<std::string, std::string> kInfuraSubdomains = {
    {brave_wallet::mojom::kMainnetChainId, "mainnet"},
    {brave_wallet::mojom::kGoerliChainId, "goerli"},
    {brave_wallet::mojom::kSepoliaChainId, "sepolia"}};

const base::flat_set<std::string> kInfuraChains = {
    brave_wallet::mojom::kMainnetChainId,
    brave_wallet::mojom::kPolygonMainnetChainId,
    brave_wallet::mojom::kOptimismMainnetChainId,
    brave_wallet::mojom::kAuroraMainnetChainId,
    brave_wallet::mojom::kAvalancheMainnetChainId,
    brave_wallet::mojom::kSepoliaChainId,
    brave_wallet::mojom::kGoerliChainId};

// DEPRECATED 01/2024. For migration only.
const base::flat_map<std::string, std::string> kSolanaSubdomains = {
    {brave_wallet::mojom::kSolanaMainnet, "mainnet"},
    {brave_wallet::mojom::kSolanaTestnet, "testnet"},
    {brave_wallet::mojom::kSolanaDevnet, "devnet"}};

// DEPRECATED 01/2024. For migration only.
const base::flat_map<std::string, std::string> kFilecoinSubdomains = {
    {brave_wallet::mojom::kFilecoinMainnet, "mainnet"},
    {brave_wallet::mojom::kFilecoinTestnet, "testnet"}};

// DEPRECATED 01/2024. For migration only.
const base::flat_map<std::string, std::string> kBitcoinSubdomains = {
    {mojom::kBitcoinMainnet, "mainnet"},
    {mojom::kBitcoinTestnet, "testnet"}};

// DEPRECATED 01/2024. For migration only.
const base::flat_map<std::string, std::string> kZCashSubdomains = {
    {mojom::kZCashMainnet, "mainnet"},
    {mojom::kZCashTestnet, "testnet"}};

// Addresses taken from https://docs.unstoppabledomains.com/developer-toolkit/
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
  const auto& custom_networks = prefs->GetDict(kBraveWalletCustomNetworks);
  return custom_networks.FindList(GetPrefKeyForCoinType(coin));
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
      if (custom_chain &&
          base::CompareCaseInsensitiveASCII(custom_chain->chain_id,
                                            known_chain->chain_id) == 0) {
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
    if (custom_chain) {
      result.push_back(std::move(custom_chain));
    }
  }

  return result;
}

std::string GetCurrentChainId(PrefService* prefs, mojom::CoinType coin) {
  const auto& selected_networks = prefs->GetDict(kBraveWalletSelectedNetworks);
  const std::string* chain_id =
      selected_networks.FindString(GetPrefKeyForCoinType(coin));
  if (!chain_id) {
    return std::string();
  }

  return base::ToLowerASCII(*chain_id);
}

mojom::BlockchainTokenPtr NetworkToNativeToken(
    const mojom::NetworkInfo& network) {
  auto result = mojom::BlockchainToken::New();

  result->chain_id = network.chain_id;
  result->coin = network.coin;
  result->name = network.symbol_name;
  result->symbol = network.symbol;
  result->decimals = network.decimals;
  result->logo = network.icon_urls.empty() ? "" : network.icon_urls[0];
  result->visible = true;

  return result;
}

// Get the address to be used in user assets API.
// For EVM, convert the address to a checksum address.
// For Solana, verify if address is a base58 encoded address, if so, return it.
// static
std::optional<std::string> GetUserAssetAddress(const std::string& address,
                                               mojom::CoinType coin,
                                               const std::string& chain_id) {
  if (address.empty()) {  // native asset
    return address;
  }

  if (coin == mojom::CoinType::ETH) {
    return EthAddress::ToEip1191ChecksumAddress(address, chain_id);
  }

  if (coin == mojom::CoinType::SOL) {
    std::vector<uint8_t> bytes;
    if (!::brave_wallet::IsBase58EncodedSolanaPubkey(address)) {
      return std::nullopt;
    }
    return address;
  }

  return std::nullopt;
}

bool ShouldCheckTokenId(const brave_wallet::mojom::BlockchainTokenPtr& token) {
  return token->is_erc721 || token->is_erc1155;
}

bool TokenMatchesDict(const brave_wallet::mojom::BlockchainTokenPtr& token,
                      const base::Value::Dict* dict) {
  if (!dict) {
    return false;
  }

  std::optional<int> coin = dict->FindInt("coin");
  if (!coin || *coin != static_cast<int>(token->coin)) {
    return false;
  }

  const std::string* chain_id = dict->FindString("chain_id");
  if (!chain_id || *chain_id != token->chain_id) {
    return false;
  }

  const std::string* address_value = dict->FindString("address");
  if (!address_value || !base::EqualsCaseInsensitiveASCII(
                            *address_value, token->contract_address)) {
    return false;
  }

  if (ShouldCheckTokenId(token)) {
    const std::string* token_id_ptr = dict->FindString("token_id");
    return token_id_ptr && *token_id_ptr == token->token_id;
  } else {
    return true;
  }
}

bool ValidateAndFixAssetAddress(mojom::BlockchainTokenPtr& token) {
  if (auto address = GetUserAssetAddress(token->contract_address, token->coin,
                                         token->chain_id)) {
    token->contract_address = *address;
    return true;
  }

  return false;
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
  if (!url.is_valid()) {
    return GURL();
  }
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
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) != 0) {
        continue;
      }

      auto result = network->Clone();
      if (result->rpc_endpoints.empty()) {
        result->active_rpc_endpoint_index = 0;
        result->rpc_endpoints = {GURL(GetInfuraURLForKnownChainId(chain_id))};
      }

      if (prefs && base::CompareCaseInsensitiveASCII(
                       chain_id, brave_wallet::mojom::kLocalhostChainId) == 0) {
        result->is_eip1559 = prefs->GetBoolean(kSupportEip1559OnLocalhostChain);
      }

      return result;
    }
    return nullptr;
  }
  if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->Clone();
      }
    }
    return nullptr;
  }
  if (coin == mojom::CoinType::SOL) {
    for (const auto* network : GetKnownSolNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->Clone();
      }
    }
    return nullptr;
  }
  if (coin == mojom::CoinType::BTC) {
    for (const auto* network : GetKnownBitcoinNetworks()) {
      if (base::EqualsCaseInsensitiveASCII(network->chain_id, chain_id)) {
        return network->Clone();
      }
    }
    return nullptr;
  }
  if (coin == mojom::CoinType::ZEC) {
    for (const auto* network : GetKnownZCashNetworks()) {
      if (base::EqualsCaseInsensitiveASCII(network->chain_id, chain_id)) {
        return network->Clone();
      }
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
  if (!custom_list) {
    return nullptr;
  }
  for (const auto& it : *custom_list) {
    if (auto opt_chain_id =
            brave_wallet::ExtractChainIdFromValue(it.GetIfDict())) {
      if (base::CompareCaseInsensitiveASCII(chain_id, *opt_chain_id) == 0) {
        return brave_wallet::ValueToNetworkInfo(it);
      }
    }
  }
  return nullptr;
}

mojom::NetworkInfoPtr GetChain(PrefService* prefs,
                               const std::string& chain_id,
                               mojom::CoinType coin) {
  if (chain_id.empty()) {
    return nullptr;
  }
  if (auto custom_chain = GetCustomChain(prefs, chain_id, coin)) {
    DCHECK(!custom_chain->supported_keyrings.empty());
    return custom_chain;
  }
  if (auto known_chain = GetKnownChain(prefs, chain_id, coin)) {
    DCHECK(!known_chain->supported_keyrings.empty());
    return known_chain;
  }

  return nullptr;
}

GURL GetInfuraURLForKnownChainId(const std::string& chain_id) {
  auto endpoint = brave_wallet::GetInfuraEndpointForKnownChainId(chain_id);
  if (!endpoint.empty()) {
    return GURL(endpoint);
  }

  auto subdomain = brave_wallet::GetInfuraSubdomainForKnownChainId(chain_id);
  if (subdomain.empty()) {
    return GURL();
  }
  return GURL(
      base::StringPrintf("https://%s-infura.brave.com/", subdomain.c_str()));
}

std::string GetInfuraEndpointForKnownChainId(const std::string& chain_id) {
  const auto& endpoints = GetInfuraChainEndpoints();
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (endpoints.contains(chain_id_lower)) {
    return endpoints.at(chain_id_lower);
  }
  return std::string();
}

std::string GetInfuraSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kInfuraSubdomains.contains(chain_id_lower)) {
    return kInfuraSubdomains.at(chain_id_lower);
  }
  return std::string();
}

// DEPRECATED 01/2024. For migration only.
std::string GetSolanaSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kSolanaSubdomains.contains(chain_id_lower)) {
    return kSolanaSubdomains.at(chain_id_lower);
  }
  return std::string();
}

// DEPRECATED 01/2024. For migration only.
std::string GetFilecoinSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kFilecoinSubdomains.contains(chain_id_lower)) {
    return kFilecoinSubdomains.at(chain_id_lower);
  }
  return std::string();
}

// DEPRECATED 01/2024. For migration only.
std::string GetBitcoinSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kBitcoinSubdomains.contains(chain_id_lower)) {
    return kBitcoinSubdomains.at(chain_id_lower);
  }
  return std::string();
}

// DEPRECATED 01/2024. For migration only.
std::string GetZCashSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kZCashSubdomains.contains(chain_id_lower)) {
    return kZCashSubdomains.at(chain_id_lower);
  }
  return std::string();
}

std::vector<mojom::NetworkInfoPtr> GetAllCustomChains(PrefService* prefs,
                                                      mojom::CoinType coin) {
  std::vector<mojom::NetworkInfoPtr> result;
  auto* custom_list = GetCustomNetworksList(prefs, coin);
  if (!custom_list) {
    return result;
  }

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
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else if (coin == mojom::CoinType::SOL) {
    for (const auto* network : GetKnownSolNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else if (coin == mojom::CoinType::BTC) {
    for (const auto* network : GetKnownBitcoinNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else if (coin == mojom::CoinType::ZEC) {
    for (const auto* network : GetKnownZCashNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else {
    NOTREACHED() << coin;
  }
  return false;
}

bool CustomChainExists(PrefService* prefs,
                       const std::string& custom_chain_id,
                       mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs, coin);
  if (!custom_list) {
    return false;
  }
  for (const auto& it : *custom_list) {
    if (auto chain_id = ExtractChainIdFromValue(it.GetIfDict())) {
      if (base::CompareCaseInsensitiveASCII(*chain_id, custom_chain_id) == 0) {
        return true;
      }
    }
  }
  return false;
}

// Returns the subset of custom_chain_ids that are custom chains
std::vector<std::string> CustomChainsExist(
    PrefService* prefs,
    const std::vector<std::string>& custom_chain_ids,
    mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs, coin);
  std::vector<std::string> existing_chain_ids;

  if (!custom_list) {
    return existing_chain_ids;
  }

  for (const auto& it : *custom_list) {
    if (auto chain_id = ExtractChainIdFromValue(it.GetIfDict())) {
      for (const auto& custom_chain_id : custom_chain_ids) {
        if (base::CompareCaseInsensitiveASCII(*chain_id, custom_chain_id) ==
            0) {
          existing_chain_ids.push_back(custom_chain_id);
          break;
        }
      }
    }
  }
  return existing_chain_ids;
}

std::string GenerateMnemonic(size_t entropy_size) {
  if (!IsValidEntropySize(entropy_size)) {
    return "";
  }

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
  if (!IsValidMnemonic(mnemonic)) {
    return nullptr;
  }

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
  if (!IsValidMnemonic(mnemonic)) {
    return nullptr;
  }

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
  if (!base::IsStringUTF8(input)) {
    return false;
  }

  if (input.empty()) {
    *output =
        "0x0000000000000000000000000000000000000000000000000000000000000000";
    return true;
  }

  // Encode count for this string
  bool success =
      PadHexEncodedParameter(Uint256ValueToHex(input.size()), output);
  if (!success) {
    return false;
  }

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
  if (!success) {
    return false;
  }

  // Write offsets to array elements.
  size_t data_offset = input.size() * 32;  // Offset to first element.
  std::string encoded_offset;
  success =
      PadHexEncodedParameter(Uint256ValueToHex(data_offset), &encoded_offset);
  if (!success) {
    return false;
  }
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
    if (!success) {
      return false;
    }
    *output += encoded_offset_for_element.substr(2);
  }

  // Write count and encoding for array elements.
  for (const auto& entry : input) {
    std::string encoded_string;
    success = EncodeString(entry, &encoded_string);
    if (!success) {
      return false;
    }
    *output += encoded_string.substr(2);
  }

  return true;
}

bool DecodeString(size_t offset,
                  const std::string& input,
                  std::string* output) {
  if (!output->empty()) {
    return false;
  }

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

// Updates preferences for when the wallet is unlocked.
// This is done in a utils function instead of in the KeyringService
// because we call it both from the old extension and the new wallet when
// it unlocks.
void UpdateLastUnlockPref(PrefService* prefs) {
  prefs->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
}

bool HasCreatedWallets(PrefService* prefs) {
  return !prefs->GetTime(kBraveWalletLastUnlockTime).is_null();
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

std::optional<TransactionReceipt> ValueToTransactionReceipt(
    const base::Value::Dict& value) {
  TransactionReceipt tx_receipt;
  const std::string* transaction_hash = value.FindString("transaction_hash");
  if (!transaction_hash) {
    return std::nullopt;
  }
  tx_receipt.transaction_hash = *transaction_hash;

  const std::string* transaction_index = value.FindString("transaction_index");
  if (!transaction_index) {
    return std::nullopt;
  }
  uint256_t transaction_index_uint;
  if (!HexValueToUint256(*transaction_index, &transaction_index_uint)) {
    return std::nullopt;
  }
  tx_receipt.transaction_index = transaction_index_uint;

  const std::string* block_hash = value.FindString("block_hash");
  if (!block_hash) {
    return std::nullopt;
  }
  tx_receipt.block_hash = *block_hash;

  const std::string* block_number = value.FindString("block_number");
  if (!block_number) {
    return std::nullopt;
  }
  uint256_t block_number_uint;
  if (!HexValueToUint256(*block_number, &block_number_uint)) {
    return std::nullopt;
  }
  tx_receipt.block_number = block_number_uint;

  const std::string* from = value.FindString("from");
  if (!from) {
    return std::nullopt;
  }
  tx_receipt.from = *from;

  const std::string* to = value.FindString("to");
  if (!to) {
    return std::nullopt;
  }
  tx_receipt.to = *to;

  const std::string* cumulative_gas_used =
      value.FindString("cumulative_gas_used");
  if (!cumulative_gas_used) {
    return std::nullopt;
  }
  uint256_t cumulative_gas_used_uint;
  if (!HexValueToUint256(*cumulative_gas_used, &cumulative_gas_used_uint)) {
    return std::nullopt;
  }
  tx_receipt.cumulative_gas_used = cumulative_gas_used_uint;

  const std::string* gas_used = value.FindString("gas_used");
  if (!gas_used) {
    return std::nullopt;
  }
  uint256_t gas_used_uint;
  if (!HexValueToUint256(*gas_used, &gas_used_uint)) {
    return std::nullopt;
  }
  tx_receipt.gas_used = gas_used_uint;

  const std::string* contract_address = value.FindString("contract_address");
  if (!contract_address) {
    return std::nullopt;
  }
  tx_receipt.contract_address = *contract_address;

  // TODO(darkdh): logs
  const std::string* logs_bloom = value.FindString("logs_bloom");
  if (!logs_bloom) {
    return std::nullopt;
  }
  tx_receipt.logs_bloom = *logs_bloom;

  std::optional<bool> status = value.FindBool("status");
  if (!status) {
    return std::nullopt;
  }
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
    for (const auto* network : GetKnownSolNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  if (coin == mojom::CoinType::BTC) {
    for (const auto* network : GetKnownBitcoinNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  if (coin == mojom::CoinType::ZEC) {
    for (const auto* network : GetKnownZCashNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  NOTREACHED();
  return result;
}

GURL GetNetworkURL(PrefService* prefs,
                   const std::string& chain_id,
                   mojom::CoinType coin) {
  if (auto custom_chain = GetCustomChain(prefs, chain_id, coin)) {
    return MaybeAddInfuraProjectId(GetActiveEndpointUrl(*custom_chain));
  } else if (auto known_chain = GetKnownChain(prefs, chain_id, coin)) {
    return MaybeAddInfuraProjectId(GetActiveEndpointUrl(*known_chain));
  }
  return GURL();
}

std::vector<mojom::NetworkInfoPtr> GetAllChains(PrefService* prefs,
                                                mojom::CoinType coin) {
  return MergeKnownAndCustomChains(GetAllKnownChains(prefs, coin),
                                   GetAllCustomChains(prefs, coin));
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownEthNetworkId(const std::string& chain_id) {
  auto subdomain = GetInfuraSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  // For known networks not in kInfuraSubdomains:
  //   localhost: Use the first RPC URL.
  //   other: Use chain ID like other custom networks.
  for (const auto* network : GetKnownEthNetworks()) {
    if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) != 0) {
      continue;
    }
    if (base::CompareCaseInsensitiveASCII(chain_id, mojom::kLocalhostChainId) ==
        0) {
      return network->rpc_endpoints.front().spec();
    }
    return base::ToLowerASCII(chain_id);
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownSolNetworkId(const std::string& chain_id) {
  auto subdomain = GetSolanaSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  // Separate check for localhost in known networks as it is predefined but
  // does not have predefined subdomain.
  if (base::CompareCaseInsensitiveASCII(chain_id, mojom::kLocalhostChainId) ==
      0) {
    for (const auto* network : GetKnownSolNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->rpc_endpoints.front().spec();
      }
    }
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownFilNetworkId(const std::string& chain_id) {
  auto subdomain = GetFilecoinSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  // Separate check for localhost in known networks as it is predefined but
  // does not have predefined subdomain.
  if (base::CompareCaseInsensitiveASCII(chain_id, mojom::kLocalhostChainId) ==
      0) {
    for (const auto* network : GetKnownFilNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->rpc_endpoints.front().spec();
      }
    }
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownBtcNetworkId(const std::string& chain_id) {
  auto subdomain = GetBitcoinSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownZecNetworkId(const std::string& chain_id) {
  auto subdomain = GetZCashSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownNetworkId(mojom::CoinType coin,
                              const std::string& chain_id) {
  if (coin == mojom::CoinType::ETH) {
    return GetKnownEthNetworkId(chain_id);
  }
  if (coin == mojom::CoinType::SOL) {
    return GetKnownSolNetworkId(chain_id);
  }
  if (coin == mojom::CoinType::FIL) {
    return GetKnownFilNetworkId(chain_id);
  }
  if (coin == mojom::CoinType::BTC) {
    return GetKnownBtcNetworkId(chain_id);
  }
  if (coin == mojom::CoinType::ZEC) {
    return GetKnownZecNetworkId(chain_id);
  }
  NOTREACHED() << coin;
  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetNetworkId(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id) {
  if (chain_id.empty()) {
    return "";
  }

  std::string id = GetKnownNetworkId(coin, chain_id);
  if (!id.empty()) {
    return id;
  }

  DCHECK(prefs);
  if (coin == mojom::CoinType::ETH) {
    for (const auto& network :
         GetAllCustomChains(prefs, mojom::CoinType::ETH)) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) != 0) {
        continue;
      }
      id = chain_id;
      break;
    }
  }

  return base::ToLowerASCII(id);
}

// DEPRECATED 01/2024. For migration only.
std::optional<std::string> GetChainIdByNetworkId_DEPRECATED(
    PrefService* prefs,
    const mojom::CoinType& coin,
    const std::string& network_id) {
  if (network_id.empty()) {
    return std::nullopt;
  }
  for (const auto& network : GetAllChains(prefs, coin)) {
    if (network_id == GetNetworkId(prefs, coin, network->chain_id)) {
      return network->chain_id;
    }
  }
  return std::nullopt;
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

std::string GetDefaultBaseCryptocurrency(PrefService* prefs) {
  return prefs->GetString(kDefaultBaseCryptocurrency);
}

GURL GetUnstoppableDomainsRpcUrl(const std::string& chain_id) {
  if (base::CompareCaseInsensitiveASCII(chain_id, mojom::kMainnetChainId) ==
          0 ||
      base::CompareCaseInsensitiveASCII(chain_id,
                                        mojom::kPolygonMainnetChainId) == 0) {
    return AddInfuraProjectId(GetInfuraURLForKnownChainId(chain_id));
  }

  NOTREACHED();
  return GURL();
}

std::string GetUnstoppableDomainsProxyReaderContractAddress(
    const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kUnstoppableDomainsProxyReaderContractAddressMap.contains(
          chain_id_lower)) {
    return kUnstoppableDomainsProxyReaderContractAddressMap.at(chain_id_lower);
  }
  return "";
}

GURL GetEnsRpcUrl() {
  return AddInfuraProjectId(
      GetInfuraURLForKnownChainId(mojom::kMainnetChainId));
}

std::string GetEnsRegistryContractAddress(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  DCHECK_EQ(chain_id_lower, mojom::kMainnetChainId);
  return kEnsRegistryContractAddress;
}

GURL GetSnsRpcUrl() {
  return GetSolMainnet()->rpc_endpoints.front();
}

void AddCustomNetwork(PrefService* prefs, const mojom::NetworkInfo& chain) {
  DCHECK(prefs);
  // FIL and SOL allow custom chains only over known ones.
  DCHECK(chain.coin == mojom::CoinType::ETH ||
         KnownChainExists(chain.chain_id, chain.coin));

  {  // Update needs to be done before GetNetworkId below.
    ScopedDictPrefUpdate update(prefs, kBraveWalletCustomNetworks);
    update->EnsureList(GetPrefKeyForCoinType(chain.coin))
        ->Append(NetworkInfoToValue(chain));
  }

  if (chain.coin != mojom::CoinType::ETH) {
    return;
  }

  RemoveUserAsset(prefs, NetworkToNativeToken(chain));
  AddUserAsset(prefs, NetworkToNativeToken(chain));
}

void RemoveCustomNetwork(PrefService* prefs,
                         const std::string& chain_id_to_remove,
                         mojom::CoinType coin) {
  DCHECK(prefs);

  ScopedDictPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value::List* list = update->FindList(GetPrefKeyForCoinType(coin));
  if (!list) {
    return;
  }
  list->EraseIf([&chain_id_to_remove](const base::Value& v) {
    DCHECK(v.is_dict());
    auto* chain_id_value = v.GetDict().FindString("chainId");
    if (!chain_id_value) {
      return false;
    }
    return base::CompareCaseInsensitiveASCII(*chain_id_value,
                                             chain_id_to_remove) == 0;
  });
}

std::vector<std::string> GetHiddenNetworks(PrefService* prefs,
                                           mojom::CoinType coin) {
  std::vector<std::string> result;
  const auto& hidden_networks = prefs->GetDict(kBraveWalletHiddenNetworks);

  auto* hidden_networks_list =
      hidden_networks.FindList(GetPrefKeyForCoinType(coin));
  if (!hidden_networks_list) {
    return result;
  }

  for (const auto& it : *hidden_networks_list) {
    if (auto* chain_id = it.GetIfString()) {
      result.push_back(base::ToLowerASCII(*chain_id));
    }
  }

  return result;
}

void AddHiddenNetwork(PrefService* prefs,
                      mojom::CoinType coin,
                      const std::string& chain_id) {
  ScopedDictPrefUpdate update(prefs, kBraveWalletHiddenNetworks);
  base::Value::List* list = update->EnsureList(GetPrefKeyForCoinType(coin));
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (!base::Contains(*list, base::Value(chain_id_lower))) {
    list->Append(chain_id_lower);
  }
}

void RemoveHiddenNetwork(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id) {
  ScopedDictPrefUpdate update(prefs, kBraveWalletHiddenNetworks);
  base::Value::List* list = update->FindList(GetPrefKeyForCoinType(coin));
  if (!list) {
    return;
  }
  list->EraseIf([&](const base::Value& v) {
    auto* chain_id_string = v.GetIfString();
    if (!chain_id_string) {
      return false;
    }
    return base::CompareCaseInsensitiveASCII(*chain_id_string, chain_id) == 0;
  });
}

std::string GetCurrentChainId(PrefService* prefs,
                              mojom::CoinType coin,
                              const std::optional<url::Origin>& origin) {
  if (!origin) {
    return GetCurrentChainId(prefs, coin);
  }
  const auto& selected_networks =
      prefs->GetDict(kBraveWalletSelectedNetworksPerOrigin);
  const auto* coin_dict =
      selected_networks.FindDict(GetPrefKeyForCoinType(coin));
  if (!coin_dict) {
    return GetCurrentChainId(prefs, coin);
  }
  const auto* chain_id_str = coin_dict->FindString(origin->Serialize());
  if (!chain_id_str) {
    return GetCurrentChainId(prefs, coin);
  }

  return base::ToLowerASCII(*chain_id_str);
}

bool SetCurrentChainId(PrefService* prefs,
                       mojom::CoinType coin,
                       const std::optional<url::Origin>& origin,
                       const std::string& chain_id) {
  // We cannot switch to an unknown chain_id
  if (!KnownChainExists(chain_id, coin) &&
      !CustomChainExists(prefs, chain_id, coin)) {
    return false;
  }
  if (!origin) {
    ScopedDictPrefUpdate update(prefs, kBraveWalletSelectedNetworks);
    update->Set(GetPrefKeyForCoinType(coin), chain_id);
  } else {
    if (origin->opaque()) {
      return false;
    }
    // Only set per origin network for http/https scheme
    if (origin->scheme() == url::kHttpScheme ||
        origin->scheme() == url::kHttpsScheme) {
      ScopedDictPrefUpdate update(prefs, kBraveWalletSelectedNetworksPerOrigin);
      update->EnsureDict(GetPrefKeyForCoinType(coin))
          ->Set(origin->Serialize(), chain_id);
    } else {
      ScopedDictPrefUpdate update(prefs, kBraveWalletSelectedNetworks);
      update->Set(GetPrefKeyForCoinType(coin), chain_id);
    }
  }
  return true;
}

std::vector<mojom::BlockchainTokenPtr> GetAllUserAssets(PrefService* prefs) {
  std::vector<mojom::BlockchainTokenPtr> result;
  const auto& user_assets_list = prefs->GetList(kBraveWalletUserAssetsList);
  for (auto& asset : user_assets_list) {
    auto* token_dict = asset.GetIfDict();
    if (!token_dict) {
      continue;
    }

    if (auto token_ptr = ValueToBlockchainToken(*token_dict)) {
      result.push_back(std::move(token_ptr));
    }
  }
  return result;
}

mojom::BlockchainTokenPtr AddUserAsset(PrefService* prefs,
                                       mojom::BlockchainTokenPtr token) {
  if (!GetChain(prefs, token->chain_id, token->coin)) {
    return nullptr;
  }

  if (!ValidateAndFixAssetAddress(token)) {
    return nullptr;
  }

  if (ShouldCheckTokenId(token)) {
    uint256_t token_id_uint = 0;
    if (!HexValueToUint256(token->token_id, &token_id_uint)) {
      return nullptr;
    }
  }

  ScopedListPrefUpdate update(prefs, kBraveWalletUserAssetsList);

  for (auto& existing_asset : *update) {
    if (TokenMatchesDict(token, existing_asset.GetIfDict())) {
      return nullptr;
    }
  }

  update->Append(BlockchainTokenToValue(token));

  return token;
}

bool RemoveUserAsset(PrefService* prefs,
                     const mojom::BlockchainTokenPtr& token) {
  ScopedListPrefUpdate update(prefs, kBraveWalletUserAssetsList);

  return update->EraseIf([&token](const base::Value& value) {
    return TokenMatchesDict(token, value.GetIfDict());
  });
}

bool SetUserAssetVisible(PrefService* prefs,
                         const mojom::BlockchainTokenPtr& token,
                         bool visible) {
  ScopedListPrefUpdate update(prefs, kBraveWalletUserAssetsList);

  for (auto& token_value : *update) {
    if (TokenMatchesDict(token, token_value.GetIfDict())) {
      token_value.GetDict().Set("visible", visible);
      return true;
    }
  }

  return false;
}

bool SetAssetSpamStatus(PrefService* prefs,
                        const mojom::BlockchainTokenPtr& token,
                        bool is_spam) {
  ScopedListPrefUpdate update(prefs, kBraveWalletUserAssetsList);

  for (auto& token_value : *update) {
    if (TokenMatchesDict(token, token_value.GetIfDict())) {
      token_value.GetDict().Set("is_spam", is_spam);
      // Marking a token as spam makes it not visible and vice-versa
      token_value.GetDict().Set("visible", !is_spam);
      return true;
    }
  }

  return false;
}

std::vector<mojom::BlockchainTokenPtr> GetDefaultEthereumAssets() {
  std::vector<mojom::BlockchainTokenPtr> user_assets_list;

  mojom::BlockchainTokenPtr bat_token = NetworkToNativeToken(*GetEthMainnet());
  bat_token->contract_address = "0x0D8775F648430679A709E98d2b0Cb6250d2887EF";
  bat_token->name = "Basic Attention Token";
  bat_token->symbol = "BAT";
  bat_token->is_erc20 = true;
  bat_token->logo = "bat.png";

  for (const auto* chain : GetKnownEthNetworks()) {
    auto asset = NetworkToNativeToken(*chain);
    user_assets_list.push_back(std::move(asset));

    // ETH Mainnet token is followed by BAT token.
    if (chain->chain_id == mojom::kMainnetChainId) {
      user_assets_list.push_back(std::move(bat_token));
    }
  }

  return user_assets_list;
}

std::vector<mojom::BlockchainTokenPtr> GetDefaultSolanaAssets() {
  std::vector<mojom::BlockchainTokenPtr> user_assets_list;

  for (const auto* chain : GetKnownSolNetworks()) {
    auto asset = NetworkToNativeToken(*chain);
    asset->logo = "sol.png";
    user_assets_list.push_back(std::move(asset));
  }

  return user_assets_list;
}

std::vector<mojom::BlockchainTokenPtr> GetDefaultFilecoinAssets() {
  std::vector<mojom::BlockchainTokenPtr> user_assets_list;

  for (const auto* chain : GetKnownFilNetworks()) {
    auto asset = NetworkToNativeToken(*chain);
    asset->logo = "fil.png";
    user_assets_list.push_back(std::move(asset));
  }

  return user_assets_list;
}

std::vector<mojom::BlockchainTokenPtr> GetDefaultBitcoinAssets() {
  std::vector<mojom::BlockchainTokenPtr> user_assets_list;

  user_assets_list.push_back(GetBitcoinNativeToken(mojom::kBitcoinMainnet));
  user_assets_list.push_back(GetBitcoinNativeToken(mojom::kBitcoinTestnet));

  return user_assets_list;
}

std::vector<mojom::BlockchainTokenPtr> GetDefaultZCashAssets() {
  std::vector<mojom::BlockchainTokenPtr> user_assets_list;

  user_assets_list.push_back(GetZcashNativeToken(mojom::kZCashMainnet));
  user_assets_list.push_back(GetZcashNativeToken(mojom::kZCashTestnet));

  return user_assets_list;
}

base::Value::List GetDefaultUserAssets() {
  base::Value::List user_assets_pref;
  for (auto& asset : GetDefaultEthereumAssets()) {
    user_assets_pref.Append(BlockchainTokenToValue(asset));
  }
  for (auto& asset : GetDefaultSolanaAssets()) {
    user_assets_pref.Append(BlockchainTokenToValue(asset));
  }
  for (auto& asset : GetDefaultFilecoinAssets()) {
    user_assets_pref.Append(BlockchainTokenToValue(asset));
  }
  for (auto& asset : GetDefaultBitcoinAssets()) {
    user_assets_pref.Append(BlockchainTokenToValue(asset));
  }
  for (auto& asset : GetDefaultZCashAssets()) {
    user_assets_pref.Append(BlockchainTokenToValue(asset));
  }
  return user_assets_pref;
}

std::string GetPrefKeyForCoinType(mojom::CoinType coin) {
  switch (coin) {
    case mojom::CoinType::BTC:
      return kBitcoinPrefKey;
    case mojom::CoinType::ZEC:
      return kZCashPrefKey;
    case mojom::CoinType::ETH:
      return kEthereumPrefKey;
    case mojom::CoinType::FIL:
      return kFilecoinPrefKey;
    case mojom::CoinType::SOL:
      return kSolanaPrefKey;
  }
  NOTREACHED() << coin;
  return "";
}

// DEPRECATED 01/2024. For migration only.
std::optional<mojom::CoinType> GetCoinTypeFromPrefKey_DEPRECATED(
    const std::string& key) {
  if (key == kEthereumPrefKey) {
    return mojom::CoinType::ETH;
  } else if (key == kFilecoinPrefKey) {
    return mojom::CoinType::FIL;
  } else if (key == kSolanaPrefKey) {
    return mojom::CoinType::SOL;
  } else if (key == kBitcoinPrefKey) {
    return mojom::CoinType::BTC;
  } else if (key == kZCashPrefKey) {
    return mojom::CoinType::ZEC;
  }
  NOTREACHED() << key;
  return std::nullopt;
}

std::string eTLDPlusOne(const url::Origin& origin) {
  return net::registry_controlled_domains::GetDomainAndRegistry(
      origin, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

mojom::OriginInfoPtr MakeOriginInfo(const url::Origin& origin) {
  return mojom::OriginInfo::New(origin.Serialize(), eTLDPlusOne(origin));
}

std::string GenerateRandomHexString() {
  std::vector<uint8_t> bytes(32);
  crypto::RandBytes(bytes);
  return base::HexEncode(bytes);
}

// Returns a string used for web3_clientVersion in the form of
// Brave/v[version]
std::string GetWeb3ClientVersion() {
  return base::StringPrintf(
      "BraveWallet/v%s", version_info::GetBraveChromiumVersionNumber().c_str());
}

std::string WalletInternalErrorMessage() {
  return l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
}

mojom::BlockchainTokenPtr GetBitcoinNativeToken(const std::string& chain_id) {
  mojom::BlockchainTokenPtr result;
  if (chain_id == GetBitcoinMainnet()->chain_id) {
    result = NetworkToNativeToken(*GetBitcoinMainnet());
    result->logo = "btc.png";
    result->coingecko_id = "btc";
  } else if (chain_id == GetBitcoinTestnet()->chain_id) {
    result = NetworkToNativeToken(*GetBitcoinTestnet());
    // TODO(apaymyshev): testnet has different logo.
    result->logo = "btc.png";
    result->coingecko_id = "";
  } else {
    NOTREACHED() << chain_id;
  }

  return result;
}

mojom::BlockchainTokenPtr GetZcashNativeToken(const std::string& chain_id) {
  mojom::BlockchainTokenPtr result;
  if (chain_id == GetZCashMainnet()->chain_id) {
    result = NetworkToNativeToken(*GetZCashMainnet());
    result->logo = "zec.png";
    result->coingecko_id = "zec";
  } else if (chain_id == GetZCashTestnet()->chain_id) {
    result = NetworkToNativeToken(*GetZCashTestnet());
    result->logo = "zec.png";
    result->coingecko_id = "zec";
  } else {
    NOTREACHED() << chain_id;
  }

  return result;
}

mojom::BlowfishOptInStatus GetTransactionSimulationOptInStatus(
    PrefService* prefs) {
  return static_cast<mojom::BlowfishOptInStatus>(
      prefs->GetInteger(kBraveWalletTransactionSimulationOptInStatus));
}

void SetTransactionSimulationOptInStatus(
    PrefService* prefs,
    const mojom::BlowfishOptInStatus& status) {
  prefs->SetInteger(kBraveWalletTransactionSimulationOptInStatus,
                    static_cast<int>(status));
}

bool IsRetriableStatus(mojom::TransactionStatus status) {
  return status == mojom::TransactionStatus::Error ||
         status == mojom::TransactionStatus::Dropped;
}

}  // namespace brave_wallet

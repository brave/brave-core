/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/network_manager.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/containers/extend.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/no_destructor.h"
#include "base/not_fatal_until.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/types/cxx23_to_underlying.h"
#include "base/values.h"
#include "base/version_info/version_info.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {

struct CaseInsensitiveCompare {
  constexpr bool operator()(std::string_view lhs, std::string_view rhs) const {
    return base::CompareCaseInsensitiveASCII(lhs, rhs) < 0;
  }
};

constexpr auto kEip1559ForKnownChains =
    base::MakeFixedFlatMap<std::string_view, bool>({
        {mojom::kMainnetChainId, true},
        {mojom::kPolygonMainnetChainId, true},
        {mojom::kAvalancheMainnetChainId, true},
        {mojom::kOptimismMainnetChainId, true},
        {mojom::kSepoliaChainId, true},
        {mojom::kFilecoinEthereumMainnetChainId, true},
        {mojom::kFilecoinEthereumTestnetChainId, true},
        {mojom::kBnbSmartChainMainnetChainId, false},
        {mojom::kBaseMainnetChainId, true},
        {mojom::kNeonEVMMainnetChainId, false},
        {mojom::kLocalhostChainId, false},
    });

constexpr auto kChainSubdomains =
    base::MakeFixedFlatMap<std::string_view, std::string_view>(
        {{mojom::kMainnetChainId, "ethereum-mainnet"},
         {mojom::kSepoliaChainId, "ethereum-sepolia"},
         {mojom::kPolygonMainnetChainId, "polygon-mainnet"},
         {mojom::kOptimismMainnetChainId, "optimism-mainnet"},
         {mojom::kBaseMainnetChainId, "base-mainnet"},
         {mojom::kAvalancheMainnetChainId, "avalanche-mainnet"},
         {mojom::kBnbSmartChainMainnetChainId, "bsc-mainnet"},

         // SVM chains.
         {mojom::kSolanaMainnet, "solana-mainnet"},

         // Bitcoin chains.
         {mojom::kBitcoinMainnet, "bitcoin-mainnet"},

         // Cardano chains.
         {mojom::kCardanoMainnet, "cardano-mainnet"},
         {mojom::kCardanoTestnet, "cardano-preprod"},

         // Polkadot chains.
         {mojom::kPolkadotMainnet, "polkadot-mainnet"},
         {mojom::kPolkadotTestnet, "polkadot-westend"}},
        CaseInsensitiveCompare());

constexpr char kGanacheLocalhostURL[] = "http://localhost:7545/";
constexpr char kSolanaLocalhostURL[] = "http://localhost:8899/";
constexpr char kFilecoinLocalhostURL[] = "http://localhost:1234/rpc/v0";

enum class ToLowerCaseReason {
  kGetURLForKnownChainId,
  kGetCurrentChainIdFromPrefs,
  kIsEip1559Chain,
  kSetEip1559ForCustomChain,
  kGetHiddenNetworks,
  kAddHiddenNetwork
};

// A helper to check if there are any cases where the chain id is not lowercase,
// to investigate why these conversions are required in the first place.
// TODO(https://github.com/brave/brave-browser/issues/46940): Adding these
// dumps in all places where this conversion is being done in this file to
// better understand why this conversion is required in the first place, and
// if we can completely eliminate them.
std::string MakeChainIdLowerCase(std::string_view chain_id,
                                 ToLowerCaseReason reason) {
  if (!std::ranges::any_of(chain_id, base::IsAsciiUpper<char>)) {
    return std::string(chain_id);
  }

  // Only dumbing for M138 so it doesn't keep rolling if we forget about it
  // (hopefully we won't though).
  if (version_info::GetMajorVersionNumberAsInt() ==
      base::to_underlying(base::NotFatalUntil::M138)) {
    SCOPED_CRASH_KEY_STRING256("wallet", "MakeChainIdLowerCaseChain", chain_id);
    switch (reason) {
      case ToLowerCaseReason::kGetURLForKnownChainId:
        base::debug::DumpWithoutCrashing();
        break;
      case ToLowerCaseReason::kGetCurrentChainIdFromPrefs:
        base::debug::DumpWithoutCrashing();
        break;
      case ToLowerCaseReason::kIsEip1559Chain:
        base::debug::DumpWithoutCrashing();
        break;
      case ToLowerCaseReason::kSetEip1559ForCustomChain:
        base::debug::DumpWithoutCrashing();
        break;
      case ToLowerCaseReason::kGetHiddenNetworks:
        base::debug::DumpWithoutCrashing();
        break;
      case ToLowerCaseReason::kAddHiddenNetwork:
        base::debug::DumpWithoutCrashing();
        break;
    }
  }

  return base::ToLowerASCII(chain_id);
}

std::optional<GURL> GetURLForKnownChainId(std::string_view chain_id) {
  // TODO(https://github.com/brave/brave-browser/issues/46940): kChainSubdomains
  // has a case-insensitive lookup. This conversion to lowercase is not
  // necessary at all, but it is being kept here for the sake of checking if the
  // conversion ever is needed to begin with.
  auto chain_id_lower =
      MakeChainIdLowerCase(chain_id, ToLowerCaseReason::kGetURLForKnownChainId);
  const auto* subdomain =
      base::FindOrNull(kChainSubdomains, std::string_view(chain_id_lower));
  if (!subdomain) {
    return std::nullopt;
  }

  return GURL(absl::StrFormat("https://%s.wallet.brave.com", *subdomain));
}

const mojom::NetworkInfo* GetEthMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Ethereum Mainnet",
       {"https://etherscan.io"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "ETH",
       "Ethereum",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       {GetURLForKnownChainId(chain_id).value()},
       "MATIC",
       "MATIC",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetBscMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kBnbSmartChainMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "BNB Smart Chain",
       {"https://bscscan.com"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "BNB",
       "BNB",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       {GetURLForKnownChainId(chain_id).value()},
       "AVAX",
       "Avalanche",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       {GetURLForKnownChainId(chain_id).value()},
       "ETH",
       "Ether",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetBaseMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kBaseMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Base",
       {"https://basescan.org"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "ETH",
       "Ether",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       {GetURLForKnownChainId(chain_id).value()},
       "ETH",
       "Ethereum",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

// Precompiled networks available in native wallet.
const std::vector<const mojom::NetworkInfo*>& GetKnownEthNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetEthMainnet(),
      GetBaseMainnet(),
      GetPolygonMainnet(),
      GetBscMainnet(),
      GetOptimismMainnet(),
      GetAvalancheMainnet(),
      GetFilecoinEthereumMainnet(),
      GetNeonEVMMainnet(),
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
       {GetURLForKnownChainId(chain_id).value()},
       "SOL",
       "Solana",
       9,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
  return GURL("https://zcash.wallet.brave.com/");
}

GURL ZCashTestnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kZCashTestnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GURL("https://testnet.zec.rocks:443/");
}

GURL CardanoMainnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kCardanoMainnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GetURLForKnownChainId(mojom::kCardanoMainnet).value();
}

GURL CardanoTestnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kCardanoTestnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GetURLForKnownChainId(mojom::kCardanoTestnet).value();
}

GURL PolkadotMainnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kPolkadotMainnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GetURLForKnownChainId(mojom::kPolkadotMainnet).value();
}

GURL PolkadotTestnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kPolkadotTestnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GetURLForKnownChainId(mojom::kPolkadotTestnet).value();
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
       {GetURLForKnownChainId(chain_id).value()},
       "BTC",
       "Bitcoin",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
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
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetZCashMainnet() {
  const auto coin = mojom::CoinType::ZEC;
  const auto* chain_id = mojom::kZCashMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Zcash Mainnet",
       {"https://3xpl.com/zcash/transaction"},
       {},
       0,
       {ZCashMainnetRpcUrl()},
       "ZEC",
       "Zcash",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetZCashTestnet() {
  const auto coin = mojom::CoinType::ZEC;
  const auto* chain_id = mojom::kZCashTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Zcash Testnet",
       {"https://blockexplorer.one/zcash/testnet/tx"},
       {},
       0,
       {ZCashTestnetRpcUrl()},
       "ZEC",
       "Zcash",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetCardanoMainnet() {
  const auto coin = mojom::CoinType::ADA;
  const auto* chain_id = mojom::kCardanoMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Cardano Mainnet",
       {"https://cexplorer.io"},
       {},
       0,
       {CardanoMainnetRpcUrl()},
       "ADA",
       "Cardano",
       6,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetCardanoTestnet() {
  const auto coin = mojom::CoinType::ADA;
  const auto* chain_id = mojom::kCardanoTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Cardano Preprod Testnet",
       {"https://preprod.cexplorer.io"},
       {},
       0,
       {CardanoTestnetRpcUrl()},
       "ADA",
       "Cardano",
       6,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetPolkadotMainnet() {
  const auto coin = mojom::CoinType::DOT;
  const auto* chain_id = mojom::kPolkadotMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Polkadot Mainnet",
       {"https://polkadot.statescan.io/"},
       {},
       0,
       {PolkadotMainnetRpcUrl()},
       "DOT",
       "Polkadot",
       10,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetPolkadotTestnet() {
  const auto coin = mojom::CoinType::DOT;
  const auto* chain_id = mojom::kPolkadotTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Polkadot Westend",
       {"https://westend.subscan.io/"},
       {},
       0,
       {PolkadotTestnetRpcUrl()},
       "WND",
       "Polkadot",
       12,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
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

const std::vector<const mojom::NetworkInfo*>& GetKnownZCashNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetZCashMainnet(),
      GetZCashTestnet(),
      // clang-format on
  });
  return *networks.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownCardanoNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetCardanoMainnet(),
      GetCardanoTestnet(),
      // clang-format on
  });
  return *networks.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownPolkadotNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetPolkadotMainnet(),
      GetPolkadotTestnet(),
      // clang-format on
  });
  return *networks.get();
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
    case mojom::CoinType::ADA:
      return kCardanoPrefKey;
    case mojom::CoinType::DOT:
      return kPolkadotPrefKey;
  }
  NOTREACHED() << coin;
}

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

std::string GetCurrentChainIdFromPrefs(PrefService* prefs,
                                       mojom::CoinType coin) {
  const auto& selected_networks = prefs->GetDict(kBraveWalletSelectedNetworks);
  const std::string* chain_id =
      selected_networks.FindString(GetPrefKeyForCoinType(coin));
  if (!chain_id) {
    return std::string();
  }

  return MakeChainIdLowerCase(*chain_id,
                              ToLowerCaseReason::kGetCurrentChainIdFromPrefs);
}

std::string GetCurrentChainIdFromPrefs(
    PrefService* prefs,
    mojom::CoinType coin,
    const std::optional<::url::Origin>& origin) {
  if (!origin) {
    return GetCurrentChainIdFromPrefs(prefs, coin);
  }
  const auto& selected_networks =
      prefs->GetDict(kBraveWalletSelectedNetworksPerOrigin);
  const auto* coin_dict =
      selected_networks.FindDict(GetPrefKeyForCoinType(coin));
  if (!coin_dict) {
    return GetCurrentChainIdFromPrefs(prefs, coin);
  }
  const auto* chain_id_str = coin_dict->FindString(origin->Serialize());
  if (!chain_id_str) {
    return GetCurrentChainIdFromPrefs(prefs, coin);
  }

  return MakeChainIdLowerCase(*chain_id_str,
                              ToLowerCaseReason::kGetCurrentChainIdFromPrefs);
}

}  // namespace

NetworkManager::NetworkManager(PrefService* prefs) : prefs_(prefs) {}
NetworkManager::~NetworkManager() = default;

// static
mojom::NetworkInfoPtr NetworkManager::GetKnownChain(std::string_view chain_id,
                                                    mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    for (const auto* network : GetKnownEthNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->Clone();
      }
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

  if (coin == mojom::CoinType::ADA) {
    for (const auto* network : GetKnownCardanoNetworks()) {
      if (base::EqualsCaseInsensitiveASCII(network->chain_id, chain_id)) {
        return network->Clone();
      }
    }
    return nullptr;
  }

  if (coin == mojom::CoinType::DOT) {
    for (const auto* network : GetKnownPolkadotNetworks()) {
      if (base::EqualsCaseInsensitiveASCII(network->chain_id, chain_id)) {
        return network->Clone();
      }
    }
    return nullptr;
  }

  NOTREACHED() << coin;
}

mojom::NetworkInfoPtr NetworkManager::GetCustomChain(std::string_view chain_id,
                                                     mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs_, coin);
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

mojom::NetworkInfoPtr NetworkManager::GetChain(std::string_view chain_id,
                                               mojom::CoinType coin) {
  if (chain_id.empty()) {
    return nullptr;
  }
  if (auto custom_chain = GetCustomChain(chain_id, coin)) {
    DCHECK(!custom_chain->supported_keyrings.empty());
    return custom_chain;
  }
  if (auto known_chain = GetKnownChain(chain_id, coin)) {
    DCHECK(!known_chain->supported_keyrings.empty());
    return known_chain;
  }

  return nullptr;
}

std::vector<mojom::NetworkInfoPtr> NetworkManager::GetAllCustomChains(
    mojom::CoinType coin) {
  std::vector<mojom::NetworkInfoPtr> result;
  auto* custom_list = GetCustomNetworksList(prefs_, coin);
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

bool NetworkManager::KnownChainExists(std::string_view chain_id,
                                      mojom::CoinType coin) {
  using GetNetworksFnPtr = const std::vector<const mojom::NetworkInfo*>& (*)();

  GetNetworksFnPtr get_networks = nullptr;

  switch (coin) {
    case mojom::CoinType::ETH:
      get_networks = &GetKnownEthNetworks;
      break;
    case mojom::CoinType::SOL:
      get_networks = &GetKnownSolNetworks;
      break;
    case mojom::CoinType::FIL:
      get_networks = &GetKnownFilNetworks;
      break;
    case mojom::CoinType::BTC:
      get_networks = &GetKnownBitcoinNetworks;
      break;
    case mojom::CoinType::ZEC:
      get_networks = &GetKnownZCashNetworks;
      break;
    case mojom::CoinType::ADA:
      get_networks = &GetKnownCardanoNetworks;
      break;
    case mojom::CoinType::DOT:
      get_networks = &GetKnownPolkadotNetworks;
      break;
  }

  CHECK(get_networks);

  for (const auto* network : get_networks()) {
    if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
      return true;
    }
  }

  return false;
}

bool NetworkManager::CustomChainExists(std::string_view custom_chain_id,
                                       mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs_, coin);
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
std::vector<std::string> NetworkManager::CustomChainsExist(
    const std::vector<std::string>& custom_chain_ids,
    mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs_, coin);
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

// static
GURL NetworkManager::GetUnstoppableDomainsRpcUrl(std::string_view chain_id) {
  if (chain_id == mojom::kMainnetChainId) {
    return GetEthMainnet()->rpc_endpoints.front();
  } else if (chain_id == mojom::kPolygonMainnetChainId) {
    return GetPolygonMainnet()->rpc_endpoints.front();
  } else if (chain_id == mojom::kBaseMainnetChainId) {
    return GetBaseMainnet()->rpc_endpoints.front();
  }

  NOTREACHED();
}

// static
GURL NetworkManager::GetEnsRpcUrl() {
  return GetEthMainnet()->rpc_endpoints.front();
}

// static
GURL NetworkManager::GetSnsRpcUrl() {
  return GetSolMainnet()->rpc_endpoints.front();
}

std::vector<mojom::NetworkInfoPtr> NetworkManager::GetAllKnownChains(
    mojom::CoinType coin) {
  using GetNetworksFnPtr = const std::vector<const mojom::NetworkInfo*>& (*)();

  std::vector<mojom::NetworkInfoPtr> result;
  GetNetworksFnPtr get_networks = nullptr;

  switch (coin) {
    case mojom::CoinType::ETH:
      get_networks = &GetKnownEthNetworks;
      break;
    case mojom::CoinType::SOL:
      get_networks = &GetKnownSolNetworks;
      break;
    case mojom::CoinType::FIL:
      get_networks = &GetKnownFilNetworks;
      break;
    case mojom::CoinType::BTC:
      get_networks = &GetKnownBitcoinNetworks;
      break;
    case mojom::CoinType::ZEC:
      get_networks = &GetKnownZCashNetworks;
      break;
    case mojom::CoinType::ADA:
      get_networks = &GetKnownCardanoNetworks;
      break;
    case mojom::CoinType::DOT:
      get_networks = &GetKnownPolkadotNetworks;
      break;
  }

  CHECK(get_networks) << coin;

  for (const auto* network : get_networks()) {
    result.push_back(network->Clone());
  }
  return result;
}

GURL NetworkManager::GetNetworkURL(std::string_view chain_id,
                                   mojom::CoinType coin) {
  if (network_url_for_testing_.contains(std::string(chain_id))) {
    return network_url_for_testing_.at(std::string(chain_id));
  }

  if (auto custom_chain = GetCustomChain(chain_id, coin)) {
    return GetActiveEndpointUrl(*custom_chain);
  }

  if (auto known_chain = GetKnownChain(chain_id, coin)) {
    return GetActiveEndpointUrl(*known_chain);
  }
  return GURL();
}

GURL NetworkManager::GetNetworkURL(mojom::CoinType coin,
                                   const std::optional<url::Origin>& origin) {
  return GetNetworkURL(GetCurrentChainId(coin, origin), coin);
}

std::vector<mojom::NetworkInfoPtr> NetworkManager::GetAllChains() {
  std::vector<mojom::NetworkInfoPtr> result;
  for (auto coin : GetEnabledCoins()) {
    base::Extend(result, MergeKnownAndCustomChains(GetAllKnownChains(coin),
                                                   GetAllCustomChains(coin)));
  }
  return result;
}

bool NetworkManager::IsEip1559Chain(std::string_view chain_id) {
  auto chain_id_lwr =
      MakeChainIdLowerCase(chain_id, ToLowerCaseReason::kIsEip1559Chain);
  if (auto is_eip_1559 = prefs_->GetDict(kBraveWalletEip1559CustomChains)
                             .FindBool(chain_id_lwr)) {
    return is_eip_1559.value();
  }
  const auto* known_chain =
      base::FindOrNull(kEip1559ForKnownChains, chain_id_lwr);
  return known_chain && *known_chain;
}

void NetworkManager::SetEip1559ForCustomChain(std::string_view chain_id,
                                              std::optional<bool> is_eip1559) {
  auto chain_id_lwr = MakeChainIdLowerCase(
      chain_id, ToLowerCaseReason::kSetEip1559ForCustomChain);
  ScopedDictPrefUpdate update(prefs_, kBraveWalletEip1559CustomChains);
  if (is_eip1559.has_value()) {
    update->Set(chain_id_lwr, is_eip1559.value());
  } else {
    update->Remove(chain_id_lwr);
  }
}

void NetworkManager::AddCustomNetwork(const mojom::NetworkInfo& chain) {
  ScopedDictPrefUpdate update(prefs_, kBraveWalletCustomNetworks);
  update->EnsureList(GetPrefKeyForCoinType(chain.coin))
      ->Append(NetworkInfoToValue(chain));
}

void NetworkManager::RemoveCustomNetwork(std::string_view chain_id,
                                         mojom::CoinType coin) {
  ScopedDictPrefUpdate update(prefs_, kBraveWalletCustomNetworks);
  base::Value::List* list = update->FindList(GetPrefKeyForCoinType(coin));
  if (!list) {
    return;
  }
  bool removed = list->EraseIf([&chain_id](const base::Value& v) {
    DCHECK(v.is_dict());
    auto* chain_id_value = v.GetDict().FindString("chainId");
    if (!chain_id_value) {
      return false;
    }
    return base::EqualsCaseInsensitiveASCII(*chain_id_value, chain_id);
  });

  if (removed) {
    SetEip1559ForCustomChain(chain_id, std::nullopt);
  }
}

std::vector<std::string> NetworkManager::GetHiddenNetworks(
    mojom::CoinType coin) {
  std::vector<std::string> result;
  const auto& hidden_networks = prefs_->GetDict(kBraveWalletHiddenNetworks);

  auto* hidden_networks_list =
      hidden_networks.FindList(GetPrefKeyForCoinType(coin));
  if (!hidden_networks_list) {
    return result;
  }

  for (const auto& it : *hidden_networks_list) {
    if (auto* chain_id = it.GetIfString()) {
      result.push_back(MakeChainIdLowerCase(
          *chain_id, ToLowerCaseReason::kGetHiddenNetworks));
    }
  }

  return result;
}

void NetworkManager::AddHiddenNetwork(mojom::CoinType coin,
                                      std::string_view chain_id) {
  ScopedDictPrefUpdate update(prefs_, kBraveWalletHiddenNetworks);
  base::Value::List* list = update->EnsureList(GetPrefKeyForCoinType(coin));
  std::string chain_id_lower =
      MakeChainIdLowerCase(chain_id, ToLowerCaseReason::kAddHiddenNetwork);
  if (!base::Contains(*list, base::Value(chain_id_lower))) {
    list->Append(chain_id_lower);
  }
}

void NetworkManager::RemoveHiddenNetwork(mojom::CoinType coin,
                                         std::string_view chain_id) {
  ScopedDictPrefUpdate update(prefs_, kBraveWalletHiddenNetworks);
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

std::string NetworkManager::GetCurrentChainId(
    mojom::CoinType coin,
    const std::optional<url::Origin>& origin) {
  auto chain_id_from_prefs = GetCurrentChainIdFromPrefs(prefs_, coin, origin);
  for (auto& chain : GetAllChains()) {
    if (coin == chain->coin && base::EqualsCaseInsensitiveASCII(
                                   chain_id_from_prefs, chain->chain_id)) {
      return chain_id_from_prefs;
    }
  }
  switch (coin) {
    case mojom::CoinType::ETH:
      return mojom::kMainnetChainId;
    case mojom::CoinType::SOL:
      return mojom::kSolanaMainnet;
    case mojom::CoinType::FIL:
      return mojom::kFilecoinMainnet;
    case mojom::CoinType::BTC:
      return mojom::kBitcoinMainnet;
    case mojom::CoinType::ZEC:
      return mojom::kZCashMainnet;
    case mojom::CoinType::ADA:
      return mojom::kCardanoMainnet;
    case mojom::CoinType::DOT:
      return mojom::kPolkadotMainnet;
  }
  NOTREACHED() << coin;
}

bool NetworkManager::SetCurrentChainId(mojom::CoinType coin,
                                       const std::optional<url::Origin>& origin,
                                       std::string_view chain_id) {
  // We cannot switch to an unknown chain_id
  if (!KnownChainExists(chain_id, coin) && !CustomChainExists(chain_id, coin)) {
    return false;
  }
  if (!origin) {
    ScopedDictPrefUpdate update(prefs_, kBraveWalletSelectedNetworks);
    update->Set(GetPrefKeyForCoinType(coin), chain_id);
  } else {
    if (origin->opaque()) {
      return false;
    }
    // Only set per origin network for http/https scheme
    if (origin->scheme() == url::kHttpScheme ||
        origin->scheme() == url::kHttpsScheme) {
      ScopedDictPrefUpdate update(prefs_,
                                  kBraveWalletSelectedNetworksPerOrigin);
      update->EnsureDict(GetPrefKeyForCoinType(coin))
          ->Set(origin->Serialize(), chain_id);
    } else {
      ScopedDictPrefUpdate update(prefs_, kBraveWalletSelectedNetworks);
      update->Set(GetPrefKeyForCoinType(coin), chain_id);
    }
  }
  return true;
}

void NetworkManager::SetNetworkURLForTesting(const std::string& chain_id,
                                             GURL url) {
  CHECK_IS_TEST();
  if (url.is_valid()) {
    network_url_for_testing_[chain_id] = std::move(url);
  } else {
    network_url_for_testing_.erase(chain_id);
  }
}

}  // namespace brave_wallet

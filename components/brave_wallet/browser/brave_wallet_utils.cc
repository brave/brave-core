/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <utility>

#include "base/environment.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/vendor/bip39wally-core-native/include/wally_bip39.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/random.h"
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
  std::string project_id(BRAVE_INFURA_PROJECT_ID);
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (env->HasVar("BRAVE_INFURA_PROJECT_ID")) {
    env->GetVar("BRAVE_INFURA_PROJECT_ID", &project_id);
  }
  return project_id;
}

bool GetUseStagingInfuraEndpoint() {
  std::string project_id(BRAVE_INFURA_PROJECT_ID);
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  return env->HasVar("BRAVE_INFURA_STAGING");
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
     "Solona Mainnet Beta",
     {"https://explorer.solana.com/"},
     {},
     {"https://api.mainnet-beta.solana.com"},
     "SOL",
     "Solana",
     9,
     brave_wallet::mojom::CoinType::SOL,
     nullptr},
    {brave_wallet::mojom::kSolanaTestnet,
     "Solona Testnet",
     {"https://explorer.solana.com/?cluster=testnet"},
     {},
     {"https://api.testnet.solana.com"},
     "SOL",
     "Solana",
     9,
     brave_wallet::mojom::CoinType::SOL,
     nullptr},
    {brave_wallet::mojom::kSolanaDevnet,
     "Solona Devnet",
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
     {"https://api.node.glif.io/rpc/v0"},
     {},
     {"https://api.node.glif.io/rpc/v0"},
     "FIL",
     "Filecoin",
     18,
     brave_wallet::mojom::CoinType::FIL,
     nullptr},
    {brave_wallet::mojom::kFilecoinTestnet,
     "Filecoin Testnet",
     {"https://calibration.node.glif.io/rpc/v0"},
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

const base::flat_map<std::string, std::string> kInfuraSubdomains = {
    {brave_wallet::mojom::kMainnetChainId, "mainnet"},
    {brave_wallet::mojom::kRinkebyChainId, "rinkeby"},
    {brave_wallet::mojom::kRopstenChainId, "ropsten"},
    {brave_wallet::mojom::kGoerliChainId, "goerli"},
    {brave_wallet::mojom::kKovanChainId, "kovan"}};

const base::flat_map<std::string, std::string> kSolanaSubdomains = {
    {brave_wallet::mojom::kSolanaMainnet, "mainnet"},
    {brave_wallet::mojom::kSolanaTestnet, "testnet"},
    {brave_wallet::mojom::kSolanaDevnet, "devnet"}};

const base::flat_map<std::string, std::string>
    kUnstoppableDomainsProxyReaderContractAddressMap = {
        {brave_wallet::mojom::kMainnetChainId,
         "0xa6E7cEf2EDDEA66352Fd68E5915b60BDbb7309f5"},
        {brave_wallet::mojom::kRinkebyChainId,
         "0x3A2e74CF832cbA3d77E72708d55370119E4323a6"}};

const base::flat_map<std::string, std::string> kEnsRegistryContractAddressMap =
    {{brave_wallet::mojom::kMainnetChainId,
      "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e"},
     {brave_wallet::mojom::kRopstenChainId,
      "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e"},
     {brave_wallet::mojom::kRinkebyChainId,
      "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e"},
     {brave_wallet::mojom::kGoerliChainId,
      "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e"}};

std::string GetInfuraURLForKnownChainId(const std::string& chain_id) {
  auto subdomain = brave_wallet::GetInfuraSubdomainForKnownChainId(chain_id);
  if (subdomain.empty())
    return std::string();
  return base::StringPrintf(
      GetUseStagingInfuraEndpoint()
          ? "https://%s-staging-infura.bravesoftware.com/%s"
          : "https://%s-infura.brave.com/%s",
      subdomain.c_str(), GetInfuraProjectID().c_str());
}

GURL GetCustomChainURL(PrefService* prefs, const std::string& chain_id) {
  std::vector<brave_wallet::mojom::NetworkInfoPtr> custom_chains;
  brave_wallet::GetAllEthCustomChains(prefs, &custom_chains);
  for (const auto& it : custom_chains) {
    if (it->chain_id != chain_id)
      continue;
    return GetFirstValidChainURL(it->rpc_urls);
  }
  return GURL();
}

}  // namespace

mojom::NetworkInfoPtr GetKnownEthChain(PrefService* prefs,
                                       const std::string& chain_id) {
  for (const auto& network : kKnownEthNetworks) {
    if (network.chain_id != chain_id)
      continue;

    auto result = network.Clone();
    if (chain_id == brave_wallet::mojom::kLocalhostChainId)
      result->data->set_eth_data(mojom::NetworkInfoDataETH::New(
          prefs->GetBoolean(kSupportEip1559OnLocalhostChain)));
    if (result->rpc_urls.empty())
      result->rpc_urls.push_back(GetInfuraURLForKnownChainId(chain_id));
    return result;
  }
  return nullptr;
}

mojom::NetworkInfoPtr GetChain(PrefService* prefs,
                               const std::string& chain_id,
                               mojom::CoinType coin) {
  std::vector<mojom::NetworkInfoPtr> chains;
  GetAllChains(prefs, coin, &chains);
  for (const auto& chain : chains) {
    if (chain->chain_id == chain_id)
      return chain.Clone();
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

void GetAllEthCustomChains(PrefService* prefs,
                           std::vector<mojom::NetworkInfoPtr>* result) {
  const base::Value* custom_networks =
      prefs->GetDictionary(kBraveWalletCustomNetworks);
  if (!custom_networks)
    return;
  const base::Value* eth_custom_networks_list =
      custom_networks->FindKey(kEthereumPrefKey);
  if (!eth_custom_networks_list)
    return;
  for (const auto& it : eth_custom_networks_list->GetListDeprecated()) {
    mojom::NetworkInfoPtr chain = brave_wallet::ValueToEthNetworkInfo(it);
    if (chain)
      result->push_back(chain->Clone());
  }
}

GURL GetFirstValidChainURL(const std::vector<std::string>& chain_urls) {
  if (chain_urls.empty())
    return GURL();
  for (const std::string& spec : chain_urls) {
    GURL url(spec);
    if (spec.find("${INFURA_API_KEY}") == std::string::npos &&
        spec.find("${ALCHEMY_API_KEY}") == std::string::npos &&
        spec.find("${API_KEY}") == std::string::npos &&
        spec.find("${PULSECHAIN_API_KEY}") == std::string::npos &&
        url.SchemeIsHTTPOrHTTPS()) {
      return url;
    }
  }
  return GURL(chain_urls.front());
}

bool IsNativeWalletEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kNativeBraveWalletFeature);
}

bool IsFilecoinEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kBraveWalletFilecoinFeature);
}

bool IsSolanaEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kBraveWalletSolanaFeature);
}

const std::vector<brave_wallet::mojom::NetworkInfoPtr>
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
    std::string encoded_offset;
    size_t rows = std::ceil(input[i - 1].size() / 32.0);
    data_offset += (rows + 1) * 32;

    success =
        PadHexEncodedParameter(Uint256ValueToHex(data_offset), &encoded_offset);
    if (!success)
      return false;
    *output += encoded_offset.substr(2);
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

void GetAllKnownEthChains(PrefService* prefs,
                          std::vector<mojom::NetworkInfoPtr>* chains) {
  for (const auto& network : kKnownEthNetworks) {
    chains->push_back(GetKnownEthChain(prefs, network.chain_id));
  }
}

GURL GetNetworkURL(PrefService* prefs,
                   const std::string& chain_id,
                   mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    mojom::NetworkInfoPtr known_network = GetKnownEthChain(prefs, chain_id);
    if (!known_network)
      return GetCustomChainURL(prefs, chain_id);

    if (known_network->rpc_urls.size())
      return GURL(known_network->rpc_urls.front());
  } else if (coin == mojom::CoinType::SOL) {
    for (const auto& network : kKnownSolNetworks) {
      if (network.chain_id == chain_id && network.rpc_urls.size()) {
        return GURL(network.rpc_urls.front());
      }
    }
  } else if (coin == mojom::CoinType::FIL) {
    for (const auto& network : kKnownFilNetworks) {
      if (network.chain_id == chain_id && network.rpc_urls.size()) {
        return GURL(network.rpc_urls.front());
      }
    }
  }
  return GURL();
}

void GetAllChains(PrefService* prefs,
                  mojom::CoinType coin,
                  std::vector<mojom::NetworkInfoPtr>* result) {
  if (coin == mojom::CoinType::ETH) {
    GetAllKnownEthChains(prefs, result);
    GetAllEthCustomChains(prefs, result);
  } else if (coin == mojom::CoinType::SOL) {
    GetAllKnownSolChains(result);
  } else if (coin == mojom::CoinType::FIL) {
    for (const auto& network : kKnownFilNetworks) {
      result->push_back(network.Clone());
    }
  }
}

void GetAllKnownSolChains(std::vector<mojom::NetworkInfoPtr>* result) {
  DCHECK(result);
  for (const auto& network : kKnownSolNetworks)
    result->push_back(network.Clone());
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

  // Separate check for localhost in known networks as it is predefined but
  // does not have infura subdomain.
  if (chain_id == mojom::kLocalhostChainId) {
    for (const auto& network : kKnownEthNetworks) {
      if (network.chain_id == chain_id) {
        return GURL(network.rpc_urls.front()).spec();
      }
    }
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

std::string GetKnownNetworkId(mojom::CoinType coin,
                              const std::string& chain_id) {
  if (coin == mojom::CoinType::ETH)
    return GetKnownEthNetworkId(chain_id);
  if (coin == mojom::CoinType::SOL)
    return GetKnownSolNetworkId(chain_id);
  // TODO(spylogsster): Implement this for FIL
  //  if (coin == mojom::CoinType::FIL)
  //    return GetKnownFilNetworkId(chain_id);
  return "";
}

std::string GetNetworkId(PrefService* prefs,
                         mojom::CoinType coin,
                         const std::string& chain_id) {
  DCHECK(prefs);

  std::string id = GetKnownNetworkId(coin, chain_id);
  if (!id.empty())
    return id;

  if (coin == mojom::CoinType::ETH) {
    std::vector<mojom::NetworkInfoPtr> custom_chains;
    GetAllEthCustomChains(prefs, &custom_chains);
    for (const auto& network : custom_chains) {
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

std::string GetDefaultBaseCryptocurrency(PrefService* prefs) {
  return prefs->GetString(kDefaultBaseCryptocurrency);
}

std::string GetUnstoppableDomainsProxyReaderContractAddress(
    const std::string& chain_id) {
  if (kUnstoppableDomainsProxyReaderContractAddressMap.contains(chain_id))
    return kUnstoppableDomainsProxyReaderContractAddressMap.at(chain_id);
  return "";
}

std::string GetEnsRegistryContractAddress(const std::string& chain_id) {
  if (kEnsRegistryContractAddressMap.contains(chain_id))
    return kEnsRegistryContractAddressMap.at(chain_id);
  return "";
}

void AddCustomNetwork(PrefService* prefs, mojom::NetworkInfoPtr chain) {
  DCHECK(prefs);

  absl::optional<base::Value> value =
      brave_wallet::EthNetworkInfoToValue(chain);
  if (!value)
    return;

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
    list->Append(std::move(value.value()));
  }

  const std::string network_id =
      GetNetworkId(prefs, mojom::CoinType::ETH, chain->chain_id);
  DCHECK(!network_id.empty());  // Not possible for a custom network.

  DictionaryPrefUpdate update(prefs, kBraveWalletUserAssets);
  base::Value* user_assets_pref = update.Get();
  base::Value* asset_list = user_assets_pref->SetKey(
      network_id, base::Value(base::Value::Type::LIST));

  base::Value native_asset(base::Value::Type::DICTIONARY);
  native_asset.SetStringKey("contract_address", "");
  native_asset.SetStringKey("name", chain->symbol_name);
  native_asset.SetStringKey("symbol", chain->symbol);
  native_asset.SetBoolKey("is_erc20", false);
  native_asset.SetBoolKey("is_erc721", false);
  native_asset.SetIntKey("decimals", chain->decimals);
  native_asset.SetBoolKey("visible", true);
  native_asset.SetStringKey(
      "logo", chain->icon_urls.empty() ? "" : chain->icon_urls[0]);

  asset_list->Append(std::move(native_asset));
}

void RemoveCustomNetwork(PrefService* prefs,
                         const std::string& chain_id_to_remove) {
  DCHECK(prefs);

  DictionaryPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value* dict = update.Get();
  CHECK(dict);
  base::Value* list = dict->FindKey(kEthereumPrefKey);
  list->EraseListValueIf([&](const base::Value& v) {
    auto* chain_id_value = v.FindStringKey("chainId");
    if (!chain_id_value)
      return false;
    return *chain_id_value == chain_id_to_remove;
  });
}

std::string GetCurrentChainId(PrefService* prefs, mojom::CoinType coin) {
  const base::Value* selected_networks =
      prefs->GetDictionary(kBraveWalletSelectedNetworks);
  DCHECK(selected_networks);
  auto pref_key = GetPrefKeyForCoinType(coin);
  if (!pref_key)
    return std::string();
  const std::string* chain_id = selected_networks->FindStringKey(*pref_key);
  if (!chain_id)
    return std::string();

  return *chain_id;
}

absl::optional<std::string> GetPrefKeyForCoinType(mojom::CoinType coin) {
  switch (coin) {
    case mojom::CoinType::ETH:
      return kEthereumPrefKey;
    case mojom::CoinType::FIL:
      return kFilecoinPrefKey;
    case mojom::CoinType::SOL:
      return kSolanaPrefKey;
  }
  return absl::nullopt;
}

}  // namespace brave_wallet

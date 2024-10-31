/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

#include <cmath>
#include <cstdlib>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/constants/brave_services_key.h"
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
  result->spl_token_program = mojom::SPLTokenProgram::kUnsupported;

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

  // ZCash shielded tokens are hardcoded, so they don't appear in the prefs
  if (token->is_shielded) {
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

bool IsEndpointUsingBraveWalletProxy(const GURL& url) {
  return url.DomainIs("wallet.brave.com") ||
         url.DomainIs("wallet.bravesoftware.com") ||
         url.DomainIs("wallet.s.brave.io");
}

base::flat_map<std::string, std::string> MakeBraveServicesKeyHeaders() {
  return {
      {kBraveServicesKeyHeader, BUILDFLAG(BRAVE_SERVICES_KEY)},
  };
}

std::string GenerateMnemonic(size_t entropy_size) {
  if (!IsValidEntropySize(entropy_size)) {
    return "";
  }

  std::vector<uint8_t> entropy(entropy_size);
  crypto::RandBytes(entropy);

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
      NOTREACHED_IN_MIGRATION();
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

std::string GetUnstoppableDomainsProxyReaderContractAddress(
    const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kUnstoppableDomainsProxyReaderContractAddressMap.contains(
          chain_id_lower)) {
    return kUnstoppableDomainsProxyReaderContractAddressMap.at(chain_id_lower);
  }
  return "";
}

std::string GetEnsRegistryContractAddress(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  DCHECK_EQ(chain_id_lower, mojom::kMainnetChainId);
  return kEnsRegistryContractAddress;
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

#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    result.push_back(GetZcashNativeShieldedToken(mojom::kZCashMainnet));
    result.push_back(GetZcashNativeShieldedToken(mojom::kZCashTestnet));
  }
#endif
  return result;
}

mojom::BlockchainTokenPtr GetUserAsset(PrefService* prefs,
                                       mojom::CoinType coin,
                                       const std::string& chain_id,
                                       const std::string& address,
                                       const std::string& token_id,
                                       bool is_erc721,
                                       bool is_erc1155,
                                       bool is_shielded) {
  mojom::BlockchainTokenPtr token = mojom::BlockchainToken::New();
  token->chain_id = chain_id;
  token->contract_address = address;
  token->coin = coin;
  token->token_id = token_id;
  token->is_erc721 = is_erc721;
  token->is_erc1155 = is_erc1155;
  token->is_shielded = is_shielded;

  // ZCash shielded tokens are hardcoded, so they don't appear in the prefs
  if (token->is_shielded && token->coin == mojom::CoinType::ZEC) {
    return GetZcashNativeShieldedToken(token->chain_id);
  }

  const auto& user_assets_list = prefs->GetList(kBraveWalletUserAssetsList);
  for (auto& asset : user_assets_list) {
    auto* token_dict = asset.GetIfDict();
    if (!token_dict) {
      continue;
    }

    if (TokenMatchesDict(token, token_dict)) {
      return ValueToBlockchainToken(*token_dict);
    }
  }

  return nullptr;
}

mojom::BlockchainTokenPtr AddUserAsset(PrefService* prefs,
                                       mojom::BlockchainTokenPtr token) {
  if (!ValidateAndFixAssetAddress(token)) {
    return nullptr;
  }

  if (ShouldCheckTokenId(token)) {
    uint256_t token_id_uint = 0;
    if (!HexValueToUint256(token->token_id, &token_id_uint)) {
      return nullptr;
    }
  }

  if (!IsSPLToken(token)) {
    token->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
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

void EnsureNativeTokenForNetwork(PrefService* prefs,
                                 const mojom::NetworkInfo& network_info) {
  auto token = NetworkToNativeToken(network_info);
  RemoveUserAsset(prefs, token);
  AddUserAsset(prefs, std::move(token));
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

bool SetAssetSPLTokenProgram(PrefService* prefs,
                             const mojom::BlockchainTokenPtr& token,
                             mojom::SPLTokenProgram program) {
  ScopedListPrefUpdate update(prefs, kBraveWalletUserAssetsList);

  for (auto& token_value : *update) {
    if (TokenMatchesDict(token, token_value.GetIfDict())) {
      token_value.GetDict().Set("spl_token_program", static_cast<int>(program));
      return true;
    }
  }

  return false;
}

bool SetAssetCompressed(PrefService* prefs,
                        const mojom::BlockchainTokenPtr& token) {
  // Only Solana tokens can be compressed.
  if (token->coin != mojom::CoinType::SOL) {
    return false;
  }

  ScopedListPrefUpdate update(prefs, kBraveWalletUserAssetsList);
  for (auto& token_value : *update) {
    if (TokenMatchesDict(token, token_value.GetIfDict())) {
      token_value.GetDict().Set("is_compressed", true);
      return true;
    }
  }

  return false;
}

std::vector<mojom::BlockchainTokenPtr> GetDefaultEthereumAssets() {
  std::vector<mojom::BlockchainTokenPtr> user_assets_list;

  for (const auto& chain :
       NetworkManager::GetAllKnownChains(mojom::CoinType::ETH)) {
    auto asset = NetworkToNativeToken(*chain);
    user_assets_list.push_back(std::move(asset));

    // ETH Mainnet token is followed by BAT token.
    if (chain->chain_id == mojom::kMainnetChainId) {
      mojom::BlockchainTokenPtr bat_token = NetworkToNativeToken(*chain);
      bat_token->contract_address = mojom::kBatTokenContractAddress;
      bat_token->name = "Basic Attention Token";
      bat_token->symbol = "BAT";
      bat_token->is_erc20 = true;
      bat_token->logo = "bat.png";
      user_assets_list.push_back(std::move(bat_token));
    }
  }

  return user_assets_list;
}

std::vector<mojom::BlockchainTokenPtr> GetDefaultSolanaAssets() {
  std::vector<mojom::BlockchainTokenPtr> user_assets_list;

  for (const auto& chain :
       NetworkManager::GetAllKnownChains(mojom::CoinType::SOL)) {
    auto asset = NetworkToNativeToken(*chain);
    asset->logo = "sol.png";
    user_assets_list.push_back(std::move(asset));
  }

  return user_assets_list;
}

std::vector<mojom::BlockchainTokenPtr> GetDefaultFilecoinAssets() {
  std::vector<mojom::BlockchainTokenPtr> user_assets_list;

  for (const auto& chain :
       NetworkManager::GetAllKnownChains(mojom::CoinType::FIL)) {
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
  NOTREACHED_IN_MIGRATION() << coin;
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
  auto network = NetworkManager::GetKnownChain(chain_id, mojom::CoinType::BTC);
  CHECK(network);

  auto result = NetworkToNativeToken(*network);
  // TODO(apaymyshev): testnet has different logo.
  result->logo = "btc.png";
  if (chain_id == mojom::kBitcoinMainnet) {
    result->coingecko_id = "btc";
  } else {
    result->coingecko_id = "";
  }

  return result;
}

mojom::BlockchainTokenPtr GetZcashNativeToken(const std::string& chain_id) {
  auto network = NetworkManager::GetKnownChain(chain_id, mojom::CoinType::ZEC);
  CHECK(network);

  auto result = NetworkToNativeToken(*network);
  result->logo = "zec.png";
  result->coingecko_id = "zec";

  return result;
}

mojom::BlockchainTokenPtr GetZcashNativeShieldedToken(
    const std::string& chain_id) {
  auto network = NetworkManager::GetKnownChain(chain_id, mojom::CoinType::ZEC);
  CHECK(network);

  auto result = NetworkToNativeToken(*network);
  result->logo = "zec.png";
  result->coingecko_id = "zec";
  result->is_shielded = true;
  result->name += "(Shielded)";

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

std::string SPLTokenProgramToProgramID(mojom::SPLTokenProgram program) {
  switch (program) {
    case mojom::SPLTokenProgram::kToken:
      return mojom::kSolanaTokenProgramId;
    case mojom::SPLTokenProgram::kToken2022:
      return mojom::kSolanaToken2022ProgramId;
    default:
      return "";
  }
}

}  // namespace brave_wallet

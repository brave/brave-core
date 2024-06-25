/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_builder.h"

#include <algorithm>
#include <map>
#include <optional>

#include "base/check.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {

bool IsValidHostLabelCharacter(char c, bool is_first_char) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || (!is_first_char && c == '-') || c == '_';
}

std::optional<std::string> ChainIdToVersion(const std::string& symbol,
                                            const std::string chain_id) {
  static base::NoDestructor<std::map<std::string, std::string>> mapping({
      {"0x1", "ERC20"},
      {"0x38", "BEP20"},
      {"0x63564c40", "HRC20"},
      {"0x64", "XDAI"},
      {"0x7a", "FUSE"},
      {"0x89", "MATIC"},
      {"0xa", "OP"},
      {"0xa4b1", "AETH"},
      {"0xa86a", "AVAX"},
      {"0xfa", "FANTOM"},
  });

  // Special case for crypto.FTM.version.OPERA.address.
  if (symbol == "FTM" && chain_id == "0xfa") {
    return "OPERA";
  }

  auto it = mapping->find(chain_id);
  if (it != mapping->end()) {
    return it->second;
  }
  return std::nullopt;
}

}  // namespace

namespace filforwarder {

constexpr uint8_t kFilForwarderSelector[] = {0xd9, 0x48, 0xd4, 0x68};

std::optional<std::vector<uint8_t>> Forward(const FilAddress& fil_address) {
  if (fil_address.IsEmpty()) {
    return std::nullopt;
  }

  return eth_abi::TupleEncoder()
      .AddBytes(fil_address.GetBytes())
      .EncodeWithSelector(base::make_span(kFilForwarderSelector));
}

}  // namespace filforwarder

namespace erc20 {

bool Transfer(const std::string& to_address,
              uint256_t amount,
              std::string* data) {
  const std::string function_hash =
      GetFunctionHash("transfer(address,uint256)");
  std::string padded_address;
  if (!brave_wallet::PadHexEncodedParameter(to_address, &padded_address)) {
    return false;
  }
  std::string padded_amount;
  if (!PadHexEncodedParameter(Uint256ValueToHex(amount), &padded_amount)) {
    return false;
  }
  std::vector<std::string> hex_strings = {function_hash, padded_address,
                                          padded_amount};
  return ConcatHexStrings(hex_strings, data);
}

bool BalanceOf(const std::string& address, std::string* data) {
  const std::string function_hash = GetFunctionHash("balanceOf(address)");
  std::string params;
  if (!brave_wallet::PadHexEncodedParameter(address, &params)) {
    return false;
  }
  return brave_wallet::ConcatHexStrings(function_hash, params, data);
}

bool Approve(const std::string& spender_address,
             uint256_t amount,
             std::string* data) {
  const std::string function_hash = GetFunctionHash("approve(address,uint256)");
  std::string padded_address;
  if (!brave_wallet::PadHexEncodedParameter(spender_address, &padded_address)) {
    return false;
  }
  std::string padded_amount;
  if (!PadHexEncodedParameter(Uint256ValueToHex(amount), &padded_amount)) {
    return false;
  }
  std::vector<std::string> hex_strings = {function_hash, padded_address,
                                          padded_amount};
  return ConcatHexStrings(hex_strings, data);
}

bool Allowance(const std::string& owner_address,
               const std::string& spender_address,
               std::string* data) {
  const std::string function_hash =
      GetFunctionHash("allowance(address,address)");
  std::string padded_owner_address;
  if (!brave_wallet::PadHexEncodedParameter(owner_address,
                                            &padded_owner_address)) {
    return false;
  }
  std::string padded_spender_address;
  if (!brave_wallet::PadHexEncodedParameter(spender_address,
                                            &padded_spender_address)) {
    return false;
  }
  std::vector<std::string> hex_strings = {function_hash, padded_owner_address,
                                          padded_spender_address};
  return ConcatHexStrings(hex_strings, data);
}

}  // namespace erc20

namespace erc721 {

bool TransferFromOrSafeTransferFrom(bool is_safe_transfer_from,
                                    const std::string& from,
                                    const std::string& to,
                                    uint256_t token_id,
                                    std::string* data) {
  const std::string function_hash =
      is_safe_transfer_from
          ? GetFunctionHash("safeTransferFrom(address,address,uint256)")
          : GetFunctionHash("transferFrom(address,address,uint256)");

  std::string padded_from;
  if (!brave_wallet::PadHexEncodedParameter(from, &padded_from)) {
    return false;
  }

  std::string padded_to;
  if (!brave_wallet::PadHexEncodedParameter(to, &padded_to)) {
    return false;
  }

  std::string padded_token_id;
  if (!PadHexEncodedParameter(Uint256ValueToHex(token_id), &padded_token_id)) {
    return false;
  }

  std::vector<std::string> hex_strings = {function_hash, padded_from, padded_to,
                                          padded_token_id};
  return ConcatHexStrings(hex_strings, data);
}

bool OwnerOf(uint256_t token_id, std::string* data) {
  const std::string function_hash = GetFunctionHash("ownerOf(uint256)");

  std::string padded_token_id;
  if (!PadHexEncodedParameter(Uint256ValueToHex(token_id), &padded_token_id)) {
    return false;
  }

  return brave_wallet::ConcatHexStrings(function_hash, padded_token_id, data);
}

bool TokenUri(uint256_t token_id, std::string* data) {
  const std::string function_hash = GetFunctionHash("tokenURI(uint256)");

  std::string padded_token_id;
  if (!PadHexEncodedParameter(Uint256ValueToHex(token_id), &padded_token_id)) {
    return false;
  }

  return brave_wallet::ConcatHexStrings(function_hash, padded_token_id, data);
}

}  // namespace erc721

namespace erc1155 {

bool SafeTransferFrom(const std::string& from,
                      const std::string& to,
                      uint256_t token_id,
                      uint256_t value,
                      std::string* data) {
  const std::string function_hash = GetFunctionHash(
      "safeTransferFrom(address,address,uint256,uint256,bytes)");

  std::string padded_from;
  if (!PadHexEncodedParameter(from, &padded_from)) {
    return false;
  }

  std::string padded_to;
  if (!PadHexEncodedParameter(to, &padded_to)) {
    return false;
  }

  std::string padded_token_id;
  if (!PadHexEncodedParameter(Uint256ValueToHex(token_id), &padded_token_id)) {
    return false;
  }

  std::string padded_value;
  if (!PadHexEncodedParameter(Uint256ValueToHex(value), &padded_value)) {
    return false;
  }

  // SafeTransferFrom's `data` parameter is arbitrary bytes that the ERC1155
  // contract will send as part of a onERC1155Received call if the recipient
  // is an contract that implements ERC1155TokenReceiver.
  // https://eips.ethereum.org/EIPS/eip-1155#erc-1155-token-receiver
  //
  // The receiver_data_arg is hardcoded as empty bytes to support basic
  // transfers only. It consists of two 32 byte parts. The first 32 bytes
  // specify the offset of SafeTransferFrom calldata where the parameter starts.
  // The second 32 bytes is the length of the data.
  //
  // Since the preceding four arguments in the calldata
  // (to, from, id, amount) are all of fixed size (32 bytes), we can always
  // specify 0xa0 (160) as the offset, since 32*(4+1) = 160.
  const std::string receiver_data_arg =
      "0x"
      // Offset
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // The length of the bytes
      "0000000000000000000000000000000000000000000000000000000000000000";

  std::vector<std::string> hex_strings = {function_hash, padded_from,
                                          padded_to,     padded_token_id,
                                          padded_value,  receiver_data_arg};
  return ConcatHexStrings(hex_strings, data);
}

bool BalanceOf(const std::string& owner_address,
               uint256_t token_id,
               std::string* data) {
  const std::string function_hash =
      GetFunctionHash("balanceOf(address,uint256)");
  std::string padded_address;
  if (!brave_wallet::PadHexEncodedParameter(owner_address, &padded_address)) {
    return false;
  }
  std::string padded_token_id;
  if (!PadHexEncodedParameter(Uint256ValueToHex(token_id), &padded_token_id)) {
    return false;
  }
  std::vector<std::string> hex_strings = {function_hash, padded_address,
                                          padded_token_id};
  return ConcatHexStrings(hex_strings, data);
}

bool Uri(uint256_t token_id, std::string* data) {
  const std::string function_hash = GetFunctionHash("uri(uint256)");
  std::string padded_token_id;
  if (!PadHexEncodedParameter(Uint256ValueToHex(token_id), &padded_token_id)) {
    return false;
  }

  return brave_wallet::ConcatHexStrings(function_hash, padded_token_id, data);
}

}  // namespace erc1155

namespace erc165 {

bool SupportsInterface(const std::string& interface_id, std::string* data) {
  if (!IsValidHexString(interface_id) || interface_id.length() != 10) {
    return false;
  }
  std::string padded_interface_id = interface_id + std::string(56, '0');

  const std::string function_hash =
      GetFunctionHash("supportsInterface(bytes4)");

  return ConcatHexStrings(function_hash, padded_interface_id, data);
}

std::vector<uint8_t> SupportsInterface(eth_abi::Span4 interface) {
  return eth_abi::TupleEncoder().AddFixedBytes(interface).EncodeWithSelector(
      kSupportsInterfaceBytes4);
}

}  // namespace erc165

namespace unstoppable_domains {

std::optional<std::string> GetMany(const std::vector<std::string>& keys,
                                   const std::string& domain) {
  const std::string function_hash =
      GetFunctionHash("getMany(string[],uint256)");

  std::string offset_for_array;
  if (!PadHexEncodedParameter(Uint256ValueToHex(64), &offset_for_array)) {
    return std::nullopt;
  }

  std::string tokenID = ToHex(Namehash(domain));

  std::string encoded_keys;
  if (!EncodeStringArray(keys, &encoded_keys)) {
    return std::nullopt;
  }

  std::string data;
  std::vector<std::string> hex_strings = {function_hash, offset_for_array,
                                          tokenID, encoded_keys};
  if (!ConcatHexStrings(hex_strings, &data)) {
    return std::nullopt;
  }

  return data;
}

std::vector<std::string> MakeEthLookupKeyList(const std::string& symbol,
                                              const std::string& chain_id) {
  auto upper_symbol = base::ToUpperASCII(symbol);
  std::vector<std::string> lookup_keys;
  // crypto.<TICKER>.version.<VERSION>.address
  if (auto version = ChainIdToVersion(upper_symbol, chain_id)) {
    if (!(upper_symbol == "ETH" && version == "ERC20")) {
      // No such key as 'crypto.ETH.version.ERC20.address'. 'crypto.ETH.address'
      // would be used instead.
      lookup_keys.push_back(base::StringPrintf("crypto.%s.version.%s.address",
                                               upper_symbol.c_str(),
                                               version->c_str()));
    }
  }
  // crypto.<TICKER>.address
  if (symbol != "ETH") {
    lookup_keys.push_back(
        base::StringPrintf("crypto.%s.address", upper_symbol.c_str()));
  }

  // crypto.ETH.address
  lookup_keys.push_back(kCryptoEthAddressKey);

  return lookup_keys;
}

std::vector<std::string> MakeSolLookupKeyList(const std::string& symbol) {
  std::vector<std::string> lookup_keys;
  // crypto.<TICKER>.version.SOLANA.address
  if (symbol != "SOL") {
    lookup_keys.push_back(
        base::StringPrintf("crypto.%s.version.SOLANA.address",
                           base::ToUpperASCII(symbol).c_str()));
  }

  // crypto.SOL.address
  lookup_keys.push_back(kCryptoSolAddressKey);

  return lookup_keys;
}

std::vector<std::string> MakeFilLookupKeyList() {
  std::vector<std::string> lookup_keys;

  // Only crypto.FIL.address supported.
  lookup_keys.push_back(kCryptoFilAddressKey);

  return lookup_keys;
}

std::vector<uint8_t> GetWalletAddr(const std::string& domain,
                                   mojom::CoinType coin,
                                   const std::string& symbol,
                                   const std::string& chain_id) {
  std::vector<std::string> key_list;
  switch (coin) {
    case mojom::CoinType::ETH:
      key_list = MakeEthLookupKeyList(symbol, chain_id);
      break;
    case mojom::CoinType::SOL:
      key_list = MakeSolLookupKeyList(symbol);
      break;
    case mojom::CoinType::FIL:
      key_list = MakeFilLookupKeyList();
      break;
    default:
      NOTREACHED_IN_MIGRATION();
  }

  // getMany(string[],uint256)
  return eth_abi::TupleEncoder()
      .AddStringArray(key_list)
      .AddFixedBytes(Namehash(domain))
      .EncodeWithSelector(kGetManySelector);
}

}  // namespace unstoppable_domains

namespace ens {

std::string Resolver(const std::string& domain) {
  const std::string function_hash = GetFunctionHash("resolver(bytes32)");
  std::string tokenID = ToHex(Namehash(domain));
  std::vector<std::string> hex_strings = {function_hash, tokenID};
  std::string data;
  ConcatHexStrings(hex_strings, &data);
  DCHECK(IsValidHexString(data));
  return data;
}

// https://docs.ens.domains/ens-improvement-proposals/ensip-10-wildcard-resolution#specification
// Similar to chromium's `DNSDomainFromDot` but without length limitation and
// support of terminal dot.
std::optional<std::vector<uint8_t>> DnsEncode(const std::string& dotted_name) {
  std::vector<uint8_t> result;
  result.resize(dotted_name.size() + 2);
  result.front() = '.';  // Placeholder for first label length.
  base::ranges::copy(dotted_name, result.begin() + 1);
  result.back() = '.';  // Placeholder for terminal zero byte.

  size_t last_dot_pos = 0;
  for (size_t i = 1; i < result.size(); ++i) {
    if (result[i] == '.') {
      size_t label_len = i - last_dot_pos - 1;
      if (label_len == 0 || label_len > 63) {
        return std::nullopt;
      }
      result[last_dot_pos] = static_cast<uint8_t>(label_len);
      last_dot_pos = i;
    } else if (!IsValidHostLabelCharacter(result[i], i - last_dot_pos == 1)) {
      return std::nullopt;
    }
  }
  DCHECK_EQ(last_dot_pos, result.size() - 1);
  result.back() = 0;
  return result;
}

}  // namespace ens

namespace balance_scanner {

std::optional<std::string> TokensBalance(
    const std::string& owner_address,
    const std::vector<std::string>& contract_addresses) {
  const std::string function_hash =
      GetFunctionHash("tokensBalance(address,address[])");
  std::string padded_address;
  if (!brave_wallet::PadHexEncodedParameter(owner_address, &padded_address)) {
    return std::nullopt;
  }

  // Indicate the next value that encodes the length of the address[] is 64
  // bytes
  std::string offset_for_array;
  if (!PadHexEncodedParameter(Uint256ValueToHex(64), &offset_for_array)) {
    return std::nullopt;
  }

  // Encode the length of the address[] as 64 bytes
  std::string array_length;
  if (!PadHexEncodedParameter(Uint256ValueToHex(contract_addresses.size()),
                              &array_length)) {
    return std::nullopt;
  }

  std::string data;
  std::vector<std::string> hex_strings = {function_hash, padded_address,
                                          offset_for_array, array_length};

  // Add the address[] to hex_strings by appending PadHexEncodedParameter of
  // each address to the end of the vector
  for (const auto& contract_address : contract_addresses) {
    std::string padded_contract_address;
    if (!brave_wallet::PadHexEncodedParameter(contract_address,
                                              &padded_contract_address)) {
      return std::nullopt;
    }
    hex_strings.push_back(padded_contract_address);
  }

  if (!ConcatHexStrings(hex_strings, &data)) {
    return std::nullopt;
  }

  return data;
}

}  // namespace balance_scanner

}  // namespace brave_wallet

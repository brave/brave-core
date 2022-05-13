/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_builder.h"

#include "base/logging.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

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
  if (!IsValidHexString(interface_id) || interface_id.length() != 10)
    return false;
  std::string padded_interface_id = interface_id + std::string(56, '0');

  const std::string function_hash =
      GetFunctionHash("supportsInterface(bytes4)");

  return ConcatHexStrings(function_hash, padded_interface_id, data);
}

}  // namespace erc165

namespace unstoppable_domains {

absl::optional<std::string> GetMany(const std::vector<std::string>& keys,
                                    const std::string& domain) {
  const std::string function_hash =
      GetFunctionHash("getMany(string[],uint256)");

  std::string offset_for_array;
  if (!PadHexEncodedParameter(Uint256ValueToHex(64), &offset_for_array)) {
    return absl::nullopt;
  }

  std::string tokenID = Namehash(domain);

  std::string encoded_keys;
  if (!EncodeStringArray(keys, &encoded_keys)) {
    return absl::nullopt;
  }

  std::string data;
  std::vector<std::string> hex_strings = {function_hash, offset_for_array,
                                          tokenID, encoded_keys};
  if (!ConcatHexStrings(hex_strings, &data)) {
    return absl::nullopt;
  }

  return data;
}

absl::optional<std::string> Get(const std::string& key,
                                const std::string& domain) {
  const std::string function_hash = GetFunctionHash("get(string,uint256)");

  std::string offset_for_key;
  if (!PadHexEncodedParameter(Uint256ValueToHex(64), &offset_for_key)) {
    return absl::nullopt;
  }

  std::string tokenID = Namehash(domain);

  std::string encoded_key;
  if (!EncodeString(key, &encoded_key)) {
    return absl::nullopt;
  }

  std::string data;
  std::vector<std::string> hex_strings = {function_hash, offset_for_key,
                                          tokenID, encoded_key};
  if (!ConcatHexStrings(hex_strings, &data))
    return absl::nullopt;

  return data;
}

}  // namespace unstoppable_domains

namespace ens {

bool Resolver(const std::string& domain, std::string* data) {
  const std::string function_hash = GetFunctionHash("resolver(bytes32)");
  std::string tokenID = Namehash(domain);
  std::vector<std::string> hex_strings = {function_hash, tokenID};
  return ConcatHexStrings(hex_strings, data);
}

bool ContentHash(const std::string& domain, std::string* data) {
  const std::string function_hash = GetFunctionHash("contenthash(bytes32)");
  std::string tokenID = Namehash(domain);
  std::vector<std::string> hex_strings = {function_hash, tokenID};
  return ConcatHexStrings(hex_strings, data);
}

bool Addr(const std::string& domain, std::string* data) {
  const std::string function_hash = GetFunctionHash("addr(bytes32)");
  std::string tokenID = Namehash(domain);
  std::vector<std::string> hex_strings = {function_hash, tokenID};
  return ConcatHexStrings(hex_strings, data);
}

}  // namespace ens

}  // namespace brave_wallet

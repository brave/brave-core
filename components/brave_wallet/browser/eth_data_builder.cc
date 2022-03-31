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

}  // namespace erc721

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

bool GetMany(const std::vector<std::string>& keys,
             const std::string& domain,
             std::string* data) {
  const std::string function_hash =
      GetFunctionHash("getMany(string[],uint256)");

  std::string offset_for_array;
  if (!PadHexEncodedParameter(Uint256ValueToHex(64), &offset_for_array)) {
    return false;
  }

  std::string tokenID = Namehash(domain);

  std::string encoded_keys;
  if (!EncodeStringArray(keys, &encoded_keys)) {
    return false;
  }

  std::vector<std::string> hex_strings = {function_hash, offset_for_array,
                                          tokenID, encoded_keys};
  if (!ConcatHexStrings(hex_strings, data)) {
    return false;
  }

  return true;
}

bool Get(const std::string& key, const std::string& domain, std::string* data) {
  const std::string function_hash = GetFunctionHash("get(string,uint256)");

  std::string offset_for_key;
  if (!PadHexEncodedParameter(Uint256ValueToHex(64), &offset_for_key)) {
    return false;
  }

  std::string tokenID = Namehash(domain);

  std::string encoded_key;
  if (!EncodeString(key, &encoded_key)) {
    return false;
  }

  std::vector<std::string> hex_strings = {function_hash, offset_for_key,
                                          tokenID, encoded_key};
  return ConcatHexStrings(hex_strings, data);
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

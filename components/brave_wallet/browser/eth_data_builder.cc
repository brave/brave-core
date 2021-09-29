/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_builder.h"

#include "base/logging.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

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

}  // namespace erc20

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

}  // namespace unstoppable_domains

namespace ens {

bool GetResolverAddress(const std::string& domain, std::string* data) {
  const std::string function_hash = GetFunctionHash("resolver(bytes32)");
  std::string tokenID = Namehash(domain);
  std::vector<std::string> hex_strings = {function_hash, tokenID};
  return ConcatHexStrings(hex_strings, data);
}

bool GetContentHashAddress(const std::string& domain, std::string* data) {
  const std::string function_hash = GetFunctionHash("contenthash(bytes32)");
  std::string tokenID = Namehash(domain);
  std::vector<std::string> hex_strings = {function_hash, tokenID};
  return ConcatHexStrings(hex_strings, data);
}

}  // namespace ens

}  // namespace brave_wallet

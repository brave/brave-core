/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HEX_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HEX_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

// Equivalent to web3.utils.toHex(string);
std::string ToHex(const std::string& data);
std::string ToHex(const std::vector<uint8_t>& data);
// Determines if the passed in hex string is valid
bool IsValidHexString(const std::string& hex_input);

// Pads a hex encoded parameter to 32-bytes
// i.e. 64 hex characters.
// Input must be prefixed with 0x
bool PadHexEncodedParameter(const std::string& hex_input, std::string* out);
// Takes 2 inputs prefixed by 0x and combines them into an output with a single
// 0x. For example 0x1 and 0x2 would return 0x12.
// Note thta this doesn't do any special casing like 0x and 0x will make 0x00
// and not 0x.
bool ConcatHexStrings(const std::string& hex_input1,
                      const std::string& hex_input2,
                      std::string* out);
bool ConcatHexStrings(const std::vector<std::string>& hex_inputs,
                      std::string* out);

// Takes a hex string as input and converts it to a uint256_t
bool HexValueToUint256(const std::string& hex_input, uint256_t* out);
// Takes a uint256_t and converts it to a hex string
std::string Uint256ValueToHex(uint256_t input);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HEX_UTILS_H_

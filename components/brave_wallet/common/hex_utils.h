/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HEX_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HEX_UTILS_H_

#include <string>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

// Equivalent to web3.utils.toHex(string);
std::string ToHex(const std::string& data);
std::string ToHex(base::span<const uint8_t> data);

// Returns a hex string representation of a binary buffer. The returned hex
// string will be in lower case, without the 0x prefix.
std::string HexEncodeLower(const void* bytes, size_t size);
std::string HexEncodeLower(base::span<const uint8_t> bytes);

// Determines if the passed in hex string is valid
bool IsValidHexString(const std::string& hex_input);

// Pads a hex encoded parameter to 32-bytes
// i.e. 64 hex characters.
// Input must be prefixed with 0x
bool PadHexEncodedParameter(const std::string& hex_input, std::string* out);
std::string PadHexEncodedParameter(const std::string& hex_input);

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
// Takes a hex string as input and converts it to a int256_t
bool HexValueToInt256(const std::string& hex_input, int256_t* out);
// Takes a uint256_t and converts it to a hex string
std::string Uint256ValueToHex(uint256_t input);

// Same as base::HexStringToBytes but with a 0x prefix
// base::HexStringToBytes crashes if used improperly like with
// base::HexStringToBytes(data.data() + 2... for "0x" input
// 0x is treated as success and returns an empty vector.
// It also handles values with uneven number of digits unlike
// base::HexStringToBytes.
// It also clears the output buffer unlike base::HexStringToBytes
bool PrefixedHexStringToBytes(const std::string& input,
                              std::vector<uint8_t>* bytes);
absl::optional<std::vector<uint8_t>> PrefixedHexStringToBytes(
    const std::string& input);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HEX_UTILS_H_

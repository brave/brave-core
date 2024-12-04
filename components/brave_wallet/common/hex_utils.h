/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HEX_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HEX_UTILS_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

// Equivalent to web3.utils.toHex(string);
// TODO(apaymyshev): rename it to To0xHex or something like that.
std::string ToHex(std::string_view data);
std::string ToHex(base::span<const uint8_t> data);

// Returns a hex string representation of a binary buffer. The returned hex
// string will be in lower case, without the 0x prefix.
std::string HexEncodeLower(base::span<const uint8_t> bytes);

// Determines if the passed in hex string is valid
bool IsValidHexString(std::string_view hex_input);

// Pads a hex encoded parameter to 32-bytes
// i.e. 64 hex characters.
// Input must be prefixed with 0x
bool PadHexEncodedParameter(std::string_view hex_input, std::string* out);
std::string PadHexEncodedParameter(std::string_view hex_input);

// Takes 2 inputs prefixed by 0x and combines them into an output with a single
// 0x. For example 0x1 and 0x2 would return 0x12.
// Note that this doesn't do any special casing like 0x and 0x will make 0x00
// and not 0x.
bool ConcatHexStrings(std::string_view hex_input1,
                      std::string_view hex_input2,
                      std::string* out);
bool ConcatHexStrings(const std::vector<std::string>& hex_inputs,
                      std::string* out);

// Takes a hex string as input and converts it to a uint256_t
bool HexValueToUint256(std::string_view hex_input, uint256_t* out);
// Takes a hex string as input and converts it to a int256_t
bool HexValueToInt256(std::string_view hex_input, int256_t* out);

// TODO(apamyshev): this call is misused in many places(like in conjuction with
// `PadHexEncodedParameter`). All call sites need review. Also needs better
// name.
// Takes a uint256_t and converts it to a hex string starting with the
// first significand digit. `0` results in `0x0`.
std::string Uint256ValueToHex(uint256_t input);

// Same as base::HexStringToBytes but with a 0x prefix
// base::HexStringToBytes crashes if used improperly like with
// base::HexStringToBytes(data.data() + 2... for "0x" input
// 0x is treated as success and returns an empty vector.
// It also handles values with uneven number of digits unlike
// base::HexStringToBytes.
// It also clears the output buffer unlike base::HexStringToBytes
bool PrefixedHexStringToBytes(std::string_view input,
                              std::vector<uint8_t>* bytes);
std::optional<std::vector<uint8_t>> PrefixedHexStringToBytes(
    std::string_view input);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_HEX_UTILS_H_

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_types.h"

namespace brave_wallet {

bool IsNativeWalletEnabled();
// Equivalent to web3.utils.toHex(string);
std::string ToHex(const std::string& data);
// Equivalent to web3.utils.keccak256(string)
std::string KeccakHash(const std::string& input, bool to_hex = true);
std::vector<uint8_t> KeccakHash(const std::vector<uint8_t>& input);
// Returns the hex encoding of the first 4 bytes of the hash.
// For example: keccak('balanceOf(address)')
std::string GetFunctionHash(const std::string& input);
// Pads a hex encoded parameter to 32-bytes
// i.e. 64 hex characters.
// Input must be prefixed with 0x
bool PadHexEncodedParameter(const std::string& hex_input, std::string* out);
// Determines if the passed in hex string is valid
bool IsValidHexString(const std::string& hex_input);
// Takes 2 inputs prefixed by 0x and combines them into an output with a single
// 0x. For example 0x1 and 0x2 would return 0x12.
// Note thta this doesn't do any special casing like 0x and 0x will make 0x00
// and not 0x.
bool ConcatHexStrings(const std::string& hex_input1,
                      const std::string& hex_input2,
                      std::string* out);
bool ConcatHexStrings(const std::vector<std::string>& hex_inputs,
                      std::string* out);

bool HexValueToUint256(const std::string& hex_input, uint256_t* out);
std::string Uint256ValueToHex(uint256_t input);

// Generate mnemonic from random entropy following BIP39.
// |entropy_size| should be specify in bytes
// If |entropy_size| is not in 16, 20, 24, 28, 32 range or allocation
// failure, the empty string will be returned.
std::string GenerateMnemonic(size_t entropy_size);
// Testing specific entropy
std::string GenerateMnemonicForTest(const std::vector<uint8_t>& entropy);
// Generate seed from mnemonic following BIP39.
// If allocation failed, it would return nullptr. Otherwise 512 bits seed will
// be returned.
std::unique_ptr<std::vector<uint8_t>> MnemonicToSeed(
    const std::string& mnemonic,
    const std::string& passphrase);

bool EncodeString(const std::string& input, std::string* output);
bool EncodeStringArray(const std::vector<std::string>& input,
                       std::string* output);

bool DecodeString(size_t offset, const std::string& input, std::string* output);
bool DecodeStringArray(const std::string& input,
                       std::vector<std::string>* output);

// Implement namehash algorithm based on EIP-137 spec.
// Used for converting domain names in the classic format (ex: brave.crypto) to
// an ERC-721 token for ENS and Unstoppable Domains.
std::string Namehash(const std::string& name);

// When we call memset in end of function to clean local variables
// for security reason, compiler optimizer can remove such call.
// So we use our own function for this purpose.
void SecureZeroData(void* data, size_t size);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_UTILS_H_

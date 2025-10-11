/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/cashaddr.h"

#include <openssl/sha.h>

#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

#include "base/containers/extend.h"
#include "base/notreached.h"

namespace brave_wallet::cashaddr {

namespace {
constexpr char kMainnetPrefix[] = "ecash";
constexpr char kTestnetPrefix[] = "ectest";

// The cashaddr character set for encoding.
const std::array<char, 32> CHARSET = {'q', 'p', 'z', 'r', 'y', '9', 'x', '8',
                                      'g', 'f', '2', 't', 'v', 'd', 'w', '0',
                                      's', '3', 'j', 'n', '5', '4', 'k', 'h',
                                      'c', 'e', '6', 'm', 'u', 'a', '7', 'l'};

// The cashaddr character set for decoding.
const std::array<int8_t, 128> CHARSET_REV = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, -1, 10, 17, 21, 20, 26, 30, 7,
    5,  -1, -1, -1, -1, -1, -1, -1, 29, -1, 24, 13, 25, 9,  8,  23, -1, 18, 22,
    31, 27, 19, -1, 1,  0,  3,  16, 11, 28, 12, 14, 6,  4,  2,  -1, -1, -1, -1,
    -1, -1, 29, -1, 24, 13, 25, 9,  8,  23, -1, 18, 22, 31, 27, 19, -1, 1,  0,
    3,  16, 11, 28, 12, 14, 6,  4,  2,  -1, -1, -1, -1, -1};

// Convert from one power-of-2 number base to another.
// If padding is enabled, this always return true. If not, then it returns true
// if all the bits of the input are encoded in the output.
template <int frombits, int tobits, bool pad, typename O, typename I>
bool ConvertBits(const O& outfn, I it, I end) {
  size_t acc = 0;
  size_t bits = 0;
  constexpr size_t maxv = (1 << tobits) - 1;
  constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;
  while (it != end) {
    acc = ((acc << frombits) | *it) & max_acc;
    bits += frombits;
    while (bits >= tobits) {
      bits -= tobits;
      outfn((acc >> bits) & maxv);
    }
    ++it;
  }

  if (pad) {
    if (bits) {
      outfn((acc << (tobits - bits)) & maxv);
    }
  } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
    return false;
  }

  return true;
}

// This function will compute what 8 5-bit values to XOR into the last 8 input
// values, in order to make the checksum 0. These 8 values are packed together
// in a single 40-bit integer. The higher bits correspond to earlier values.
uint64_t PolyMod(const std::vector<uint8_t>& v) {
  // The input is interpreted as a list of coefficients of a polynomial over F
  // = GF(32), with an implicit 1 in front. If the input is [v0,v1,v2,v3,v4],
  // that polynomial is v(x) = 1*x^5 + v0*x^4 + v1*x^3 + v2*x^2 + v3*x + v4.
  // The implicit 1 guarantees that [v0,v1,v2,...] has a distinct checksum
  // from [0,v0,v1,v2,...].
  //
  // The output is a 40-bit integer whose 5-bit groups are the coefficients of
  // the remainder of v(x) mod g(x), where g(x) is the cashaddr generator, x^8
  // + {19}*x^7 + {3}*x^6 + {25}*x^5 + {11}*x^4 + {25}*x^3 + {3}*x^2 + {19}*x
  // + {1}. g(x) is chosen in such a way that the resulting code is a BCH
  // code, guaranteeing detection of up to 4 errors within a window of 1025
  // characters. Among the various possible BCH codes, one was selected to in
  // fact guarantee detection of up to 5 errors within a window of 160
  // characters and 6 erros within a window of 126 characters. In addition,
  // the code guarantee the detection of a burst of up to 8 errors.
  //
  // Note that the coefficients are elements of GF(32), here represented as
  // decimal numbers between {}. In this finite field, addition is just XOR of
  // the corresponding numbers. For example, {27} + {13} = {27 ^ 13} = {22}.
  // Multiplication is more complicated, and requires treating the bits of
  // values themselves as coefficients of a polynomial over a smaller field,
  // GF(2), and multiplying those polynomials mod a^5 + a^3 + 1. For example,
  // {5} * {26} = (a^2 + 1) * (a^4 + a^3 + a) = (a^4 + a^3 + a) * a^2 + (a^4 +
  // a^3 + a) = a^6 + a^5 + a^4 + a = a^3 + 1 (mod a^5 + a^3 + 1) = {9}.
  //
  // During the course of the loop below, `c` contains the bitpacked
  // coefficients of the polynomial constructed from just the values of v that
  // were processed so far, mod g(x). In the above example, `c` initially
  // corresponds to 1 mod (x), and after processing 2 inputs of v, it
  // corresponds to x^2 + v0*x + v1 mod g(x). As 1 mod g(x) = 1, that is the
  // starting value for `c`.
  uint64_t c = 1;
  for (uint8_t d : v) {
    // We want to update `c` to correspond to a polynomial with one extra
    // term. If the initial value of `c` consists of the coefficients of
    // c(x) = f(x) mod g(x), we modify it to correspond to
    // c'(x) = (f(x) * x + d) mod g(x), where d is the next input to
    // process.
    //
    // Simplifying:
    // c'(x) = (f(x) * x + d) mod g(x)
    //         ((f(x) mod g(x)) * x + d) mod g(x)
    //         (c(x) * x + d) mod g(x)
    // If c(x) = c0*x^5 + c1*x^4 + c2*x^3 + c3*x^2 + c4*x + c5, we want to
    // compute
    // c'(x) = (c0*x^5 + c1*x^4 + c2*x^3 + c3*x^2 + c4*x + c5) * x + d
    //                                                             mod g(x)
    //       = c0*x^6 + c1*x^5 + c2*x^4 + c3*x^3 + c4*x^2 + c5*x + d
    //                                                             mod g(x)
    //       = c0*(x^6 mod g(x)) + c1*x^5 + c2*x^4 + c3*x^3 + c4*x^2 +
    //                                                             c5*x + d
    // If we call (x^6 mod g(x)) = k(x), this can be written as
    // c'(x) = (c1*x^5 + c2*x^4 + c3*x^3 + c4*x^2 + c5*x + d) + c0*k(x)

    // First, determine the value of c0:
    uint8_t c0 = c >> 35;

    // Then compute c1*x^5 + c2*x^4 + c3*x^3 + c4*x^2 + c5*x + d:
    c = ((c & 0x07ffffffff) << 5) ^ d;

    // Finally, for each set bit n in c0, conditionally add {2^n}k(x):
    if (c0 & 0x01) {
      // k(x) = {19}*x^7 + {3}*x^6 + {25}*x^5 + {11}*x^4 + {25}*x^3 +
      //        {3}*x^2 + {19}*x + {1}
      c ^= 0x98f2bc8e61;
    }

    if (c0 & 0x02) {
      // {2}k(x) = {15}*x^7 + {6}*x^6 + {27}*x^5 + {22}*x^4 + {27}*x^3 +
      //           {6}*x^2 + {15}*x + {2}
      c ^= 0x79b76d99e2;
    }

    if (c0 & 0x04) {
      // {4}k(x) = {30}*x^7 + {12}*x^6 + {31}*x^5 + {5}*x^4 + {31}*x^3 +
      //           {12}*x^2 + {30}*x + {4}
      c ^= 0xf33e5fb3c4;
    }

    if (c0 & 0x08) {
      // {8}k(x) = {21}*x^7 + {24}*x^6 + {23}*x^5 + {10}*x^4 + {23}*x^3 +
      //           {24}*x^2 + {21}*x + {8}
      c ^= 0xae2eabe2a8;
    }

    if (c0 & 0x10) {
      // {16}k(x) = {3}*x^7 + {25}*x^6 + {7}*x^5 + {20}*x^4 + {7}*x^3 +
      //            {25}*x^2 + {3}*x + {16}
      c ^= 0x1e4f43e470;
    }
  }

  // PolyMod computes what value to xor into the final values to make the
  // checksum 0. However, if we required that the checksum was 0, it would be
  // the case that appending a 0 to a valid list of values would result in a
  // new valid list. For that reason, cashaddr requires the resulting checksum
  // to be 1 instead.
  return c ^ 1;
}

// Convert a char to lower case.
inline uint8_t LowerCase(uint8_t c) {
  // ASCII black magic.
  return c | 0x20;
}

// Expand the address prefix for the checksum computation.
std::vector<uint8_t> ExpandPrefix(const std::string& prefix) {
  std::vector<uint8_t> ret;
  ret.resize(prefix.size() + 1);
  for (size_t i = 0; i < prefix.size(); ++i) {
    ret[i] = prefix[i] & 0x1f;
  }

  ret[prefix.size()] = 0;
  return ret;
}

bool VerifyChecksum(const std::string& prefix,
                    const std::vector<uint8_t>& payload) {
  std::vector<uint8_t> prefixed_payload{ExpandPrefix(prefix)};
  base::Extend(prefixed_payload, payload);
  return PolyMod(prefixed_payload) == 0;
}

std::vector<uint8_t> CreateChecksum(const std::string& prefix,
                                    const std::vector<uint8_t>& payload) {
  std::vector<uint8_t> enc{ExpandPrefix(prefix)};
  base::Extend(enc, payload);
  // Append 8 zeroes.
  enc.resize(enc.size() + 8);
  // Determine what to XOR into those 8 zeroes.
  uint64_t mod = PolyMod(enc);
  std::vector<uint8_t> ret(8);
  for (size_t i = 0; i < 8; ++i) {
    // Convert the 5-bit groups in mod to checksum values.
    ret[i] = (mod >> (5 * (7 - i))) & 0x1f;
  }

  return ret;
}

// Convert the data part to a 5 bit representation.
std::vector<uint8_t> PackAddressData(const std::vector<uint8_t>& hash,
                                     uint8_t type) {
  uint8_t version_byte(type << 3);
  size_t size = hash.size();
  uint8_t encoded_size = 0;
  switch (size * 8) {
    case 160:
      encoded_size = 0;
      break;
    case 192:
      encoded_size = 1;
      break;
    case 224:
      encoded_size = 2;
      break;
    case 256:
      encoded_size = 3;
      break;
    case 320:
      encoded_size = 4;
      break;
    case 384:
      encoded_size = 5;
      break;
    case 448:
      encoded_size = 6;
      break;
    case 512:
      encoded_size = 7;
      break;
    default:
      NOTREACHED();
  }
  version_byte |= encoded_size;
  std::vector<uint8_t> data = {version_byte};
  data.insert(data.end(), std::begin(hash), std::end(hash));

  std::vector<uint8_t> converted;
  // Reserve the number of bytes required for a 5-bit packed version of a
  // hash, with version byte.  Add half a byte(4) so integer math provides
  // the next multiple-of-5 that would fit all the data.
  converted.reserve(((size + 1) * 8 + 4) / 5);
  ConvertBits<8, 5, true>([&](uint8_t c) { converted.push_back(c); },
                          std::begin(data), std::end(data));

  return converted;
}

std::optional<ChainType> ChainTypeFromPrefix(const std::string& prefix) {
  // Attempt to detect the chain type from the prefix.
  if (prefix == kMainnetPrefix) {
    return ChainType::MAIN;
  }
  if (prefix == kTestnetPrefix) {
    return ChainType::TEST;
  }
  return std::nullopt;
}

}  // namespace

AddressContent::AddressContent(AddressType type,
                               std::vector<uint8_t> hash,
                               ChainType chain_type)
    : address_type(type), hash(std::move(hash)), chain_type(chain_type) {}

AddressContent::~AddressContent() = default;
AddressContent::AddressContent(const AddressContent&) = default;
AddressContent& AddressContent::operator=(const AddressContent&) = default;
AddressContent::AddressContent(AddressContent&&) = default;
AddressContent& AddressContent::operator=(AddressContent&&) = default;

std::string Encode(const std::string& prefix,
                   const std::vector<uint8_t>& payload) {
  std::vector<uint8_t> checksum = CreateChecksum(prefix, payload);
  std::vector<uint8_t> combined{payload};
  base::Extend(combined, checksum);
  std::string ret = prefix + ':';

  ret.reserve(ret.size() + combined.size());
  for (uint8_t c : combined) {
    ret += CHARSET.at(c);
  }

  return ret;
}

std::optional<std::pair<std::string, std::vector<uint8_t>>> Decode(
    const std::string& str,
    const std::string& default_prefix) {
  // Go over the string and do some sanity checks.
  bool lower{false};
  bool upper{false};
  bool has_number{false};
  size_t prefix_size = 0;
  for (size_t i = 0; i < str.size(); ++i) {
    uint8_t c = str[i];
    if (c >= 'a' && c <= 'z') {
      lower = true;
      continue;
    }

    if (c >= 'A' && c <= 'Z') {
      upper = true;
      continue;
    }

    if (c >= '0' && c <= '9') {
      // We cannot have numbers in the prefix.
      has_number = true;
      continue;
    }

    if (c == ':') {
      // The separator cannot be the first character, cannot have number
      // and there must not be 2 separators.
      if (has_number || i == 0 || prefix_size != 0) {
        return {};
      }

      prefix_size = i;
      continue;
    }

    // We have an unexpected character.
    return {};
  }

  // We can't have both upper case and lowercase.
  if (upper && lower) {
    return {};
  }

  // Get the prefix.
  std::string prefix;
  if (prefix_size == 0) {
    prefix = default_prefix;
  } else {
    prefix.reserve(prefix_size);
    for (size_t i = 0; i < prefix_size; ++i) {
      prefix += LowerCase(str[i]);
    }

    // Now add the ':' in the size.
    prefix_size++;
  }

  // Decode values.
  const size_t value_size = str.size() - prefix_size;
  std::vector<uint8_t> values(value_size);
  for (size_t i = 0; i < value_size; ++i) {
    uint8_t c = str[i + prefix_size];
    // We have an invalid char in there.
    if (c > 127 || CHARSET_REV.at(c) == -1) {
      return {};
    }

    values[i] = CHARSET_REV.at(c);
  }

  // Verify the checksum.
  if (!VerifyChecksum(prefix, values)) {
    return {};
  }

  return std::make_optional<std::pair<std::string, std::vector<uint8_t>>>(
      {std::move(prefix),
       std::vector<uint8_t>(values.begin(), values.end() - 8)});
}

std::string EncodeCashAddress(const std::string& prefix,
                              const AddressContent& content) {
  std::vector<uint8_t> data =
      PackAddressData(content.hash, content.address_type);
  return cashaddr::Encode(prefix, data);
}

std::optional<std::string> PrefixFromChainType(const ChainType& chain_type) {
  std::string expected_prefix;
  if (chain_type == ChainType::MAIN) {
    return kMainnetPrefix;
  }
  if (chain_type == ChainType::TEST) {
    return kTestnetPrefix;
  }
  return std::nullopt;
}

std::optional<AddressContent> DecodeCashAddress(
    const std::string& addr,
    const std::string& expected_prefix) {
  auto decoded = cashaddr::Decode(addr, expected_prefix);
  if (!decoded.has_value()) {
    return std::nullopt;
  }
  auto [prefix, payload] = decoded.value();

  if (prefix != expected_prefix) {
    return std::nullopt;
  }

  auto chain_type = ChainTypeFromPrefix(prefix);
  if (!chain_type) {
    return std::nullopt;
  }

  if (payload.empty()) {
    return std::nullopt;
  }

  std::vector<uint8_t> data;
  data.reserve(payload.size() * 5 / 8);
  if (!ConvertBits<5, 8, false>([&](uint8_t c) { data.push_back(c); },
                                begin(payload), end(payload))) {
    return std::nullopt;
  }

  // Decode type and size from the version.
  uint8_t version = data[0];
  if (version & 0x80) {
    // First bit is reserved.
    return std::nullopt;
  }

  auto type = AddressType((version >> 3) & 0x1f);
  uint32_t hash_size = 20 + 4 * (version & 0x03);
  if (version & 0x04) {
    hash_size *= 2;
  }

  // Check that we decoded the exact number of bytes we expected.
  if (data.size() != hash_size + 1) {
    return std::nullopt;
  }

  // Pop the version.
  data.erase(data.begin());
  return AddressContent(type, data, *chain_type);
}

}  // namespace brave_wallet::cashaddr

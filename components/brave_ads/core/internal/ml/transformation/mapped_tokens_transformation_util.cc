/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation_util.h"

namespace brave_ads::ml {

unsigned char GetNewZeroBits() {
  unsigned char new_bits = '0';
  int bits_in_char = sizeof(char) * CHAR_BIT;
  for (int i = 0; i < bits_in_char; i++) {
    new_bits &= ~(static_cast<unsigned char>(1) << i);
  }
  return new_bits;
}

unsigned char SetBit(unsigned char compressed_bits,
                     int bit_position,
                     int bit_value) {
  if (bit_value == 0) {
    compressed_bits &= ~(static_cast<unsigned char>(1) << bit_position);
  }
  if (bit_value == 1) {
    compressed_bits |= (static_cast<unsigned char>(1) << bit_position);
  }
  return compressed_bits;
}

absl::optional<std::basic_string<unsigned char>> CompressToken(
    const std::string& token,
    std::map<std::string, std::vector<int>> huffman_coding_mapping) {
  std::basic_string<unsigned char> compressed_token;

  unsigned char compressed_bits = GetNewZeroBits();
  int bits_in_char = sizeof(char) * CHAR_BIT;
  int bits_set = 0;
  for (const char& character : token) {
    std::string huffman_coding_key;
    huffman_coding_key.push_back(character);

    const auto iter = huffman_coding_mapping.find(huffman_coding_key);
    if (iter == huffman_coding_mapping.end()) {
      return absl::nullopt;
    }

    std::vector<int> character_bit_encoding = iter->second;
    for (const auto& bit_value : character_bit_encoding) {
      compressed_bits = SetBit(compressed_bits, bits_set, bit_value);
      bits_set += 1;
      if (bits_set == bits_in_char) {
        compressed_token.push_back(compressed_bits);
        compressed_bits = GetNewZeroBits();
        bits_set = 0;
      }
    }
  }

  if (bits_set > 0) {
    compressed_token.push_back(compressed_bits);
  }

  return compressed_token;
}

}  // namespace brave_ads::ml

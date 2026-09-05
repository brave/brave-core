/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_private_cdn/private_cdn_helper.h"

#include <array>
#include <string>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/types/zip.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BravePrivateCdnHelperTest, RemovePadding) {
  using std::string_view_literals::operator""sv;

  constexpr static auto invalid_inputs = std::to_array<std::string_view>({
      ""sv,
      "\xFF"sv,
      "\x00\x00\x00"sv,
      "\x00\x00\x00\x01"sv,
      "\x00\x00\x00\x04"sv
      "ABC"sv,
      "\x00\x00\x00\x08"sv
      "ABCDPPP"sv,
  });

  for (auto invalid_input : invalid_inputs) {
    EXPECT_FALSE(brave::private_cdn::RemovePadding(&invalid_input));
  }

  constexpr static auto inputs = std::to_array<std::string_view>({
      "\x00\x00\x00\x00"sv,
      "\x00\x00\x00\x04"sv
      "ABCD"sv,
      "\x00\x00\x01\x00"sv
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv,
      "\x00\x00\x00\x04"sv
      "ABCDPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP"sv,
      "\x00\x00\x00\x05"sv
      "AB"sv
      "\x00"sv
      "CDPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP"sv,
      "\x00\x00\x00\x01"sv
      "PP"sv,
      "\x00\x00\x00\x04"sv
      "ABCDABCD"sv,
      "\x00\x00\x00\x01"sv
      "P"sv
      "\x00\x00\x00"sv,
  });
  constexpr static auto outputs = std::to_array<std::string_view>({
      ""sv,
      "ABCD"sv,
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv,
      "ABCD"sv,
      "AB"sv
      "\x00"sv
      "CD"sv,
      "P"sv,
      "ABCD"sv,
      "P"sv,
  });

  static_assert(inputs.size() == outputs.size(),
                "inputs and outputs must have the same size");

  for (auto [input, output] : base::zip(inputs, outputs)) {
    std::string_view padded_string = input;
    EXPECT_TRUE(brave::private_cdn::RemovePadding(&padded_string));
    EXPECT_EQ(padded_string, output);
  }
}

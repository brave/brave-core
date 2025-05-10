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
  using std::string_literals::operator""s;

  constexpr static auto invalid_inputs = std::to_array<std::string_view>({
      "",
      "\xFF",
      "\x00\x00\x00",
      "\x00\x00\x00\x01",
      "\x00\x00\x00\x04"
      "ABC",
      "\x00\x00\x00\x08"
      "ABCDPPP",
  });

  for (auto invalid_input : invalid_inputs) {
    EXPECT_FALSE(brave::private_cdn::RemovePadding(&invalid_input));
  }

  constexpr static auto inputs = std::to_array<std::string_view>({
      "\x00\x00\x00\x00",
      "\x00\x00\x00\x04"
      "ABCD",
      "\x00\x00\x01\x00"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
      "\x00\x00\x00\x04"
      "ABCDPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP",
      "\x00\x00\x00\x05"
      "AB"
      "\x00"
      "CDPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP",
      "\x00\x00\x00\x01"
      "PP",
      "\x00\x00\x00\x04"
      "ABCDABCD",
      "\x00\x00\x00\x01"
      "P"
      "\x00\x00\x00",
  });
  constexpr static auto outputs = std::to_array<std::string_view>({
      "",
      "ABCD",
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
      "ABCD",
      "AB"
      "\x00"
      "CD",
      "P",
      "ABCD",
      "P",
  });

  static_assert(inputs.size() == outputs.size(),
                "inputs and outputs must have the same size");

  for (auto [input, output] : base::zip(inputs, outputs)) {
    std::string_view padded_string = input;
    EXPECT_TRUE(brave::private_cdn::RemovePadding(&padded_string));
    EXPECT_EQ(padded_string, output);
  }
}

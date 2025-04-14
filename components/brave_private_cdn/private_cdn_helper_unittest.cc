/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_private_cdn/private_cdn_helper.h"

#include <string>
#include <string_view>

#include "base/compiler_specific.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BravePrivateCdnHelper, RemovePadding) {
  using std::string_literals::operator""s;

  std::string invalid_inputs[]{
      ""s,
      "\xFF"s,
      "\x00\x00\x00"s,
      "\x00\x00\x00\x01"s,
      "\x00\x00\x00\x04"s
      "ABC"s,
      "\x00\x00\x00\x08"s
      "ABCDPPP"s,
  };

  for (auto& invalid_input : invalid_inputs) {
    std::string_view padded_string(invalid_input);
    EXPECT_FALSE(brave::private_cdn::RemovePadding(&padded_string));
  }

  std::string inputs[] = {
      "\x00\x00\x00\x00"s,
      "\x00\x00\x00\x04"s
      "ABCD"s,
      "\x00\x00\x01\x00"s
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"s
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"s
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"s
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"s,
      "\x00\x00\x00\x04"s
      "ABCDPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP"s,
      "\x00\x00\x00\x05"s
      "AB"s
      "\x00"s
      "CDPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP"s,
      "\x00\x00\x00\x01"s
      "PP"s,
      "\x00\x00\x00\x04"s
      "ABCDABCD"s,
      "\x00\x00\x00\x01"s
      "P"s
      "\x00\x00\x00"s,
  };
  const std::string outputs[]{
      ""s,
      "ABCD"s,
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"s
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"s
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"s
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"s,
      "ABCD"s,
      "AB"s
      "\x00"s
      "CD"s,
      "P"s,
      "ABCD"s,
      "P"s,
  };

  constexpr size_t kInputCount = sizeof(inputs) / sizeof(std::string);
  static_assert(kInputCount == sizeof(outputs) / sizeof(std::string),
                "Inputs and outputs must have the same number of elements.");

  for (size_t i = 0; i < kInputCount; i++) {
    std::string_view padded_string(UNSAFE_TODO(inputs[i]));
    EXPECT_TRUE(brave::private_cdn::RemovePadding(&padded_string));
    EXPECT_EQ(padded_string, UNSAFE_TODO(outputs[i]));
  }
}

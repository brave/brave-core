/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include <string>
#include <string_view>

#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BravePrivateCdnHelper, RemovePadding) {
  auto* helper = brave::PrivateCdnHelper::GetInstance();

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
    EXPECT_FALSE(helper->RemovePadding(&padded_string));
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

  constexpr size_t input_count = sizeof(inputs) / sizeof(std::string);
  static_assert(input_count == sizeof(outputs) / sizeof(std::string),
                "Inputs and outputs must have the same number of elements.");

  for (size_t i = 0; i < input_count; i++) {
    std::string_view padded_string(inputs[i]);
    EXPECT_TRUE(helper->RemovePadding(&padded_string));
    EXPECT_EQ(padded_string, outputs[i]);
  }
}

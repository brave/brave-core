// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "brave/components/brave_ads/rust/lib.rs.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::rust {

namespace {

const struct collapse_case {
  const char* input;
  const char* output;
} kCollapseCases[] = {
    {" Google Video ", "Google Video"},
    {"Google Video", "Google Video"},
    {"", ""},
    {"  ", ""},
    {"\t\rTest String\n", "Test String"},
    {"\u2002Test String\u00A0\u3000", "Test String"},
    {"    Test     \n  \t String    ", "TestString"},
    {"\u2002Test\u1680 \u2028 \tString\u00A0\u3000", "Test String"},
    {"   Test String", "Test String"},
    {"Test String    ", "Test String"},
    {"Test String", "Test String"},
    {"", ""},
    {"\n", ""},
    {"  \r  ", ""},
    {"\nFoo", "Foo"},
    {"\r  Foo  ", "Foo"},
    {" Foo bar ", "Foo bar"},
    {"  \tFoo  bar  \n", "Foo bar"},
    {" a \r b\n c \r\n d \t\re \t f \n ", "abcde f"},
};

std::string CollapseWhitespace(const std::string& text) {
  return static_cast<std::string>(collapse_whitespace(text));
}

}  // namespace

TEST(BraveAdsCollapseWhitespaceTest, CollapseWhitespace) {
  for (const auto& value : kCollapseCases) {
    EXPECT_EQ(value.output, CollapseWhitespace(value.input));
  }
}

}  // namespace brave_ads::rust

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/int_param_parser.h"

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(IntParamParserTest, ShouldReturnZeroWhenFieldIsMissing) {
  const base::DictValue dict;

  EXPECT_EQ(0, ParseIntParam(dict).value);
}

TEST(IntParamParserTest, ShouldReturnZeroWhenFieldIsNotAnInt) {
  const base::DictValue dict =
      base::DictValue().Set("value_param", "not an int");

  EXPECT_EQ(0, ParseIntParam(dict).value);
}

TEST(IntParamParserTest, ShouldReturnStoredValue) {
  const base::DictValue dict = base::DictValue().Set("value_param", 42);

  EXPECT_EQ(42, ParseIntParam(dict).value);
}

}  // namespace gestures

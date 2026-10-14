// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/text_param_parser.h"

#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(TextParamParserTest, ShouldReturnEmptyStringWhenFieldIsMissing) {
  const base::DictValue dict;

  EXPECT_THAT(ParseTextParam(dict).value, testing::IsEmpty());
}

TEST(TextParamParserTest, ShouldReturnEmptyStringWhenFieldIsNotAString) {
  const base::DictValue dict = base::DictValue().Set("text_param", 42);

  EXPECT_THAT(ParseTextParam(dict).value, testing::IsEmpty());
}

TEST(TextParamParserTest, ShouldReturnStoredValue) {
  const base::DictValue dict =
      base::DictValue().Set("text_param", "https://brave.com");

  EXPECT_EQ("https://brave.com", ParseTextParam(dict).value);
}

}  // namespace gestures

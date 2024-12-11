/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/utils.h"

#include <string>
#include <string_view>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

TEST(AIChatCommonUtilsUnitTest, IsBraveSearchURL) {
  EXPECT_TRUE(IsBraveSearchURL(GURL("https://search.brave.com")));
  EXPECT_FALSE(IsBraveSearchURL(GURL("http://search.brave.com")));
  EXPECT_FALSE(IsBraveSearchURL(GURL("https://test.brave.com/")));
  EXPECT_FALSE(IsBraveSearchURL(GURL("https://brave.com/")));
  EXPECT_FALSE(IsBraveSearchURL(GURL()));
}

TEST(AIChatCommonUtilsUnitTest, IsOpenAIChatButtonFromBraveSearchURL) {
  EXPECT_TRUE(IsOpenAIChatButtonFromBraveSearchURL(
      GURL("https://search.brave.com/leo#5566")));
  EXPECT_FALSE(IsOpenAIChatButtonFromBraveSearchURL(GURL()));
  EXPECT_FALSE(
      IsOpenAIChatButtonFromBraveSearchURL(GURL("https://search.brave.com")));
  EXPECT_FALSE(IsOpenAIChatButtonFromBraveSearchURL(
      GURL("https://search.brave.com/leo")));
  EXPECT_FALSE(IsOpenAIChatButtonFromBraveSearchURL(
      GURL("https://search.brave.com/leo#")));
  EXPECT_FALSE(
      IsOpenAIChatButtonFromBraveSearchURL(GURL("https://brave.com/leo#5566")));
}

}  // namespace ai_chat

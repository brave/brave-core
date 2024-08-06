/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

TEST(AIChatUtilsUnitTest, IsBraveSearchSERP) {
  EXPECT_TRUE(IsBraveSearchSERP(GURL("https://search.brave.com/search?q=foo")));
  // Missing or wrong path.
  EXPECT_FALSE(IsBraveSearchSERP(GURL("https://search.brave.com?q=foo")));
  EXPECT_FALSE(
      IsBraveSearchSERP(GURL("https://search.brave.com/test.html?q=foo")));
  // Missing or wrong query parameter.
  EXPECT_FALSE(IsBraveSearchSERP(GURL("https://search.brave.com/search")));
  EXPECT_FALSE(IsBraveSearchSERP(GURL("https://search.brave.com/search?t=t")));
  // HTTP scheme.
  EXPECT_FALSE(IsBraveSearchSERP(GURL("http://search.brave.com/search?q=foo")));
  // Wrong host.
  EXPECT_FALSE(IsBraveSearchSERP(GURL("https://brave.com/search?q=foo")));
}

}  // namespace ai_chat

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/common/permissions/permissions_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

TEST(BravePermissionsData, IsBraveProtectedUrl) {
  ASSERT_TRUE(extensions::IsBraveProtectedUrl(
      GURL("https://uphold.com/authorize/1234")));
  ASSERT_TRUE(extensions::IsBraveProtectedUrl(
      GURL("https://sandbox.uphold.com/authorize/1234?state=abcd")));
  ASSERT_TRUE(extensions::IsBraveProtectedUrl(
      GURL("https://api.uphold.com/oauth2/token")));

  ASSERT_FALSE(extensions::IsBraveProtectedUrl(
      GURL("https://uphold.com/")));
  ASSERT_FALSE(extensions::IsBraveProtectedUrl(
      GURL("https://www.uphold.com/")));
  ASSERT_FALSE(extensions::IsBraveProtectedUrl(
      GURL("https://brave.com/")));
}

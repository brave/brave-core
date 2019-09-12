/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_extensions_api_client.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

TEST(BraveExtensionsApiClient, IsBraveSecurityUrl) {
  ASSERT_TRUE(extensions::IsBraveSecurityUrl(
      GURL("https://sb-ssl.brave.com/something")));
  ASSERT_TRUE(extensions::IsBraveSecurityUrl(
      GURL("https://safebrowsing.brave.com/")));

  ASSERT_FALSE(extensions::IsBraveSecurityUrl(
      GURL("https://www.brave.com/")));
  ASSERT_FALSE(extensions::IsBraveSecurityUrl(
      GURL("https://brave.com/")));
}

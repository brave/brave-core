/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/security_interstitials/core/insecure_form_util_unittest.cc"

TEST_F(InsecureFormUtilTest, IsInsecureFormActionOnOnionSource) {
  // Should work even without special-casing .onion
  EXPECT_TRUE(IsInsecureFormActionOnSecureSource(
      url::Origin::Create(GURL("https://foo.onion")),
      GURL("http://example.com")));
  EXPECT_FALSE(IsInsecureFormActionOnSecureSource(
      url::Origin::Create(GURL("http://foo.onion")),
      GURL("https://example.com")));

  // Basic case
  EXPECT_TRUE(IsInsecureFormActionOnSecureSource(
      url::Origin::Create(GURL("http://foo.onion")),
      GURL("http://example.com")));

  // Subdomains
  EXPECT_TRUE(IsInsecureFormActionOnSecureSource(
      url::Origin::Create(GURL("http://foo.bar.onion")),
      GURL("http://example.com")));

  // Non-onion URLs
  EXPECT_FALSE(IsInsecureFormActionOnSecureSource(
      url::Origin::Create(GURL("http://foo.onion.com")),
      GURL("http://example.com")));
  EXPECT_FALSE(IsInsecureFormActionOnSecureSource(
      url::Origin::Create(GURL("http://foo.baronion")),
      GURL("http://example.com")));
}

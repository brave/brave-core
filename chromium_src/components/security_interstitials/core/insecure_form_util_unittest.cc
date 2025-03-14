/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/security_interstitials/core/insecure_form_util_unittest.cc"

TEST_F(InsecureFormUtilTest, IsInsecureFormActionOnOnionSource) {
  EXPECT_TRUE(IsInsecureFormActionOnSecureSource(
      url::Origin::Create(GURL("http://foo.onion")),
      GURL("http://example.com")));

  EXPECT_TRUE(IsInsecureFormActionOnSecureSource(
      url::Origin::Create(GURL("https://foo.onion")),
      GURL("http://example.com")));
}

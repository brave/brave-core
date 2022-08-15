/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/renderer/skus_utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_url.h"
#include "url/gurl.h"

namespace skus {

TEST(SkusUtilsUnitTest, IsSafeOrigin) {
  EXPECT_TRUE(skus::IsSafeOrigin(
      blink::WebSecurityOrigin::Create(GURL("https://account.brave.com"))));
  EXPECT_TRUE(skus::IsSafeOrigin(blink::WebSecurityOrigin::Create(
      GURL("https://account.bravesoftware.com"))));
  EXPECT_TRUE(skus::IsSafeOrigin(blink::WebSecurityOrigin::Create(
      GURL("https://account.brave.software"))));

  EXPECT_FALSE(skus::IsSafeOrigin(blink::WebSecurityOrigin::Create(
      GURL("https://a.account.brave.software"))));
  EXPECT_FALSE(skus::IsSafeOrigin(
      blink::WebSecurityOrigin::Create(GURL("https://vpm.brave.com"))));
  EXPECT_FALSE(skus::IsSafeOrigin(blink::WebSecurityOrigin::Create(GURL())));
  EXPECT_FALSE(skus::IsSafeOrigin(
      blink::WebSecurityOrigin::Create(GURL("http://account.brave.com"))));
  EXPECT_FALSE(skus::IsSafeOrigin(
      blink::WebSecurityOrigin::Create(GURL("https://brave.com"))));
  EXPECT_FALSE(skus::IsSafeOrigin(
      blink::WebSecurityOrigin::Create(GURL("file://brave.com"))));
}

}  // namespace skus

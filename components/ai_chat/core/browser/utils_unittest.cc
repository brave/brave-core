/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/utils.h"

#include <optional>
#include <string_view>

#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

using AIChatUtilsUnitTest = ::testing::Test;

TEST_F(AIChatUtilsUnitTest, IsBraveSearchSERP) {
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

TEST_F(AIChatUtilsUnitTest, GetBraveHeadersNoCredential) {
  auto headers = GetBraveHeaders(std::nullopt);
  EXPECT_FALSE(headers.contains("Cookie"));
  EXPECT_TRUE(headers.contains("x-brave-key"));
}

TEST_F(AIChatUtilsUnitTest, GetBraveHeadersWithCredentialNoOrderId) {
  CredentialCacheEntry credential;
  credential.credential = "test-credential";
  credential.expires_at = base::Time::Now() + base::Hours(1);

  auto headers = GetBraveHeaders(credential);
  ASSERT_TRUE(headers.contains("Cookie"));
  EXPECT_EQ(headers.at("Cookie"),
           "__Secure-sku#brave-leo-premium=test-credential");
  EXPECT_TRUE(headers.contains("x-brave-key"));
}

TEST_F(AIChatUtilsUnitTest, GetBraveHeadersWithCredentialAndOrderId) {
  CredentialCacheEntry credential;
  credential.credential = "test-credential";
  credential.expires_at = base::Time::Now() + base::Hours(1);
  credential.order_id = "test-order-id";

  auto headers = GetBraveHeaders(credential);
  ASSERT_TRUE(headers.contains("Cookie"));
  EXPECT_EQ(
      headers.at("Cookie"),
      "__Secure-sku#brave-leo-premium=test-credential; id=test-order-id");
}

TEST_F(AIChatUtilsUnitTest, GetBraveHeadersWithEmptyOrderIdOmitsIdPair) {
  CredentialCacheEntry credential;
  credential.credential = "test-credential";
  credential.expires_at = base::Time::Now() + base::Hours(1);
  credential.order_id = "";

  auto headers = GetBraveHeaders(credential);
  ASSERT_TRUE(headers.contains("Cookie"));
  EXPECT_EQ(headers.at("Cookie"),
           "__Secure-sku#brave-leo-premium=test-credential");
}

}  // namespace ai_chat

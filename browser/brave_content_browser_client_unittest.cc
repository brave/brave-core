/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_wallet/browser/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

typedef testing::Test BraveContentBrowserClientTest;

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
TEST_F(BraveContentBrowserClientTest, ResolvesEthereumRemoteClient) {
  GURL url("chrome://wallet/");
  ASSERT_TRUE(BraveContentBrowserClient::HandleURLOverrideRewrite(
        &url, nullptr));
  ASSERT_STREQ(url.spec().c_str(), ethereum_remote_client_base_url);
}
#endif

TEST_F(BraveContentBrowserClientTest, ResolvesSync) {
  GURL url("chrome://sync-internals/");
  ASSERT_TRUE(BraveContentBrowserClient::HandleURLOverrideRewrite(
        &url, nullptr));
  ASSERT_STREQ(url.spec().c_str(), "chrome://sync/");

  GURL url2("chrome://sync/");
  ASSERT_TRUE(BraveContentBrowserClient::HandleURLOverrideRewrite(
        &url2, nullptr));
}

TEST_F(BraveContentBrowserClientTest, ResolvesWelcomePage) {
  GURL url("chrome://welcome-win10/");
  ASSERT_TRUE(BraveContentBrowserClient::HandleURLOverrideRewrite(
        &url, nullptr));
  ASSERT_STREQ(url.spec().c_str(), "chrome://welcome/");

  GURL url2("chrome://welcome/");
  ASSERT_TRUE(BraveContentBrowserClient::HandleURLOverrideRewrite(
        &url2, nullptr));
}

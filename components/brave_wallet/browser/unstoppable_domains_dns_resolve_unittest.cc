/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/unstoppable_domains_dns_resolve.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet::unstoppable_domains {

class UnstoppableDomainsResolveUrlTest : public testing::Test {
 public:
  std::vector<std::string> DefaultRpcResult() {
    return {
        "QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka",  // dweb.ipfs.hash
        "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR",  // ipfs.html.value
        "",                                                // dns.A
        "",                                                // dns.AAAA
        "https://fallback1.test.com",  // browser.redirect_url
        "https://fallback2.test.com",  // ipfs.redirect_domain.value
    };
  }
};

TEST_F(UnstoppableDomainsResolveUrlTest, IncorrectArraySize) {
  std::vector<std::string> rpc_result1 = DefaultRpcResult();
  rpc_result1.pop_back();
  EXPECT_TRUE(ResolveUrl(rpc_result1).is_empty());

  std::vector<std::string> rpc_result2 = DefaultRpcResult();
  rpc_result2.push_back("");
  EXPECT_TRUE(ResolveUrl(rpc_result2).is_empty());

  std::vector<std::string> rpc_result3 = {};
  EXPECT_TRUE(ResolveUrl(rpc_result3).is_empty());
}

TEST_F(UnstoppableDomainsResolveUrlTest, Default) {
  EXPECT_EQ(GURL("https://ipfs.io/ipfs/"
                 "QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka"),
            ResolveUrl(DefaultRpcResult()));
}

TEST_F(UnstoppableDomainsResolveUrlTest, FallbackToIpfsHtmlValue) {
  auto rpc_result = DefaultRpcResult();
  rpc_result[0] = "";
  EXPECT_EQ(GURL("https://ipfs.io/ipfs/"
                 "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR"),
            ResolveUrl(rpc_result));
}

TEST_F(UnstoppableDomainsResolveUrlTest, FallbackToBrowserRedirectUrl) {
  auto rpc_result = DefaultRpcResult();
  rpc_result[0] = "";
  rpc_result[1] = "";
  EXPECT_EQ(GURL("https://fallback1.test.com"), ResolveUrl(rpc_result));
}

TEST_F(UnstoppableDomainsResolveUrlTest, FallbackToIpfsRedirectDomainValue) {
  auto rpc_result = DefaultRpcResult();
  rpc_result[0] = "";
  rpc_result[1] = "";
  rpc_result[4] = "";
  EXPECT_EQ(GURL("https://fallback2.test.com"), ResolveUrl(rpc_result));
}

}  // namespace brave_wallet::unstoppable_domains

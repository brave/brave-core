/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/wallet_connect/wallet_connect_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace wallet_connect {

TEST(WalletConnectUtilsUnitTest, ParseWalletConnectURI) {
  auto result = ParseWalletConnectURI(
      "wc:ceee161c-29fa-433f-9f8b-27bfb5157f6e@1?bridge=https%3A%2F%2Fk.bridge."
      "walletconnect.org&key="
      "a641a11b4254de4b55b9fdc7ccede1aba7fa618405d106f5a7251998698fa1bf");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->topic, "ceee161c-29fa-433f-9f8b-27bfb5157f6e");
  EXPECT_EQ(result->version, 1);
  ASSERT_TRUE(result->params->is_v1_params());
  EXPECT_EQ(result->params->get_v1_params()->key,
            "a641a11b4254de4b55b9fdc7ccede1aba7fa618405d106f5a7251998698fa1bf");
  EXPECT_EQ(result->params->get_v1_params()->bridge,
            GURL("https://k.bridge.walletconnect.org"));

  auto result2 = ParseWalletConnectURI(
      "wc:c9e6d30fb34afe70a15c14e9337ba8e4d5a35dd695c39b94884b0ee60c69d168@2?"
      "relay-protocol=waku&symKey="
      "7ff3e362f825ab868e20e767fe580d0311181632707e7c878cbeca0238d45b8b");
  ASSERT_TRUE(result2);
  EXPECT_EQ(result2->topic,
            "c9e6d30fb34afe70a15c14e9337ba8e4d5a35dd695c39b94884b0ee60c69d168");
  EXPECT_EQ(result2->version, 2);
  ASSERT_TRUE(result2->params->is_v2_params());
  EXPECT_EQ(result2->params->get_v2_params()->sym_key,
            "7ff3e362f825ab868e20e767fe580d0311181632707e7c878cbeca0238d45b8b");
  EXPECT_EQ(result2->params->get_v2_params()->relay_protocol, "waku");

  // invalid cases
  for (const std::string& c :
       {// wrong scheme
        "mail:ceee161c-29fa-433f-9f8b-27bfb5157f6e@1?bridge=https%3A%2F%2Fk."
        "bridge.walletconnect.org&key="
        "a641a11b4254de4b55b9fdc7ccede1aba7fa618405d106f5a7251998698fa1bf",
        // unsupported version
        "wc:ceee161c-29fa-433f-9f8b-27bfb5157f6e@5?bridge=https%3A%2F%2Fk."
        "bridge.walletconnect.org&key="
        "a641a11b4254de4b55b9fdc7ccede1aba7fa618405d106f5a7251998698fa1bf",
        // version is not number
        "wc:ceee161c-29fa-433f-9f8b-27bfb5157f6e@brave?bridge=https%3A%2F%2Fk."
        "bridge.walletconnect.org&key="
        "a641a11b4254de4b55b9fdc7ccede1aba7fa618405d106f5a7251998698fa1bf",
        // wrong paths
        "wc:ceee161c-29fa-433f-9f8b-27bfb5157f6e@1@2@3?bridge=https%3A%2F%2Fk."
        "bridge.walletconnect.org&key="
        "a641a11b4254de4b55b9fdc7ccede1aba7fa618405d106f5a7251998698fa1bf",
        // missing param
        "wc:ceee161c-29fa-433f-9f8b-27bfb5157f6e@2?bridge=https%3A%2F%2Fk."
        "bridge.walletconnect.org",
        // invalid bridge URL
        "wc:ceee161c-29fa-433f-9f8b-27bfb5157f6e@2?bridge=invalid&key="
        "a641a11b4254de4b55b9fdc7ccede1aba7fa618405d106f5a7251998698fa1bf",
        // v2 with v1 params
        "wc:ceee161c-29fa-433f-9f8b-27bfb5157f6e@2?bridge=https%3A%2F%2Fk."
        "bridge.walletconnect.org&key="
        "a641a11b4254de4b55b9fdc7ccede1aba7fa618405d106f5a7251998698fa1bf",
        // v1 with v2 params
        "wc:c9e6d30fb34afe70a15c14e9337ba8e4d5a35dd695c39b94884b0ee60c69d168@"
        "1?relay-protocol=waku&symKey="
        "7ff3e362f825ab868e20e767fe580d0311181632707e7c878cbeca0238d45b8b"}) {
    EXPECT_FALSE(ParseWalletConnectURI(c));
  }
}

}  // namespace wallet_connect

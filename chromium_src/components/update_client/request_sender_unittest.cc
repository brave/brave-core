/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
#include "testing/gtest/include/gtest/gtest.h"

// We use our own hardcoded CUP signing key (see
// chromium_src/components/update_client/request_sender.cc), bypassing
// upstream's ECDSA/ML-DSA-44 (PQC) key-selection logic entirely, so this
// assertion of upstream key selection never holds in our builds.
#define CupKeySelection DISABLED_CupKeySelection

#include <components/update_client/request_sender_unittest.cc>

#undef CupKeySelection

namespace update_client {

class RequestSenderTest;

TEST_P(RequestSenderTest, UsesBraveCUPKey) {
  EXPECT_TRUE(post_interceptor_->ExpectRequest(
      std::make_unique<PartialMatch>("test"),
      GetTestFilePath("updatecheck_reply_1.json")));

  const std::vector<GURL> urls = {GURL(kUrl1)};
  request_sender_ =
      base::MakeRefCounted<RequestSender>(config_->GetNetworkFetcherFactory());
  request_sender_->Send(
      urls, {}, "test", true,
      base::BindOnce(&RequestSenderTest::RequestSenderComplete,
                     base::Unretained(this)));
  RunThreads();

  EXPECT_EQ(1, post_interceptor_->GetHitCount())
      << post_interceptor_->GetRequestsAsString();
  GURL request_url = std::get<2>(post_interceptor_->GetRequests()[0]);
  // It's hard to check the key contents. But it is easy to check the key
  // version. Ours differs from upstream. So we can use this as a proxy check
  // that our key is indeed being used.
  EXPECT_NE(request_url.query().find("cup2key=1:"), std::string::npos)
      << request_url.query();
}

// This is our replacement for the disabled upstream `CupKeySelection` test. No
// matter what key upstream's `kPqcCupSigning` flag would have selected,
// `RequestSender` always signs with Brave's own key.
TEST_P(RequestSenderTest, BraveKeyIgnoresPqcCupSigningFlag) {
  EXPECT_TRUE(post_interceptor_->ExpectRequest(
      std::make_unique<PartialMatch>("test"),
      GetTestFilePath("updatecheck_reply_1.json")));

  const std::vector<GURL> urls = {GURL(kUrl1)};
  request_sender_ =
      base::MakeRefCounted<RequestSender>(config_->GetNetworkFetcherFactory());
  request_sender_->Send(
      urls, {}, "test", true,
      base::BindOnce(&RequestSenderTest::RequestSenderComplete,
                     base::Unretained(this)));
  RunThreads();

  const std::string query(
      std::get<2>(post_interceptor_->GetRequests()[0]).query());
  EXPECT_TRUE(base::StartsWith(query, "cup2key=1:")) << query;
  EXPECT_FALSE(base::StartsWith(query, "cup2key=16:")) << query;
  EXPECT_FALSE(base::StartsWith(query, "cup2key=ML-DSA-44-16:")) << query;
}

}  // namespace update_client

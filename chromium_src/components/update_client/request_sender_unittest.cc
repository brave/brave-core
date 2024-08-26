/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/update_client/features.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "src/components/update_client/request_sender_unittest.cc"

namespace update_client {

class RequestSenderTest;

class BraveRequestSenderTest : public RequestSenderTest {
 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  bool UsesBraveCUPKey() {
    EXPECT_TRUE(post_interceptor_->ExpectRequest(
        std::make_unique<PartialMatch>("test"),
        GetTestFilePath("updatecheck_reply_1.json")));

    const std::vector<GURL> urls = {GURL(kUrl1)};
    request_sender_ = std::make_unique<RequestSender>(config_);
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
    // that our key (which currently has version 1) is indeed being used.
    return request_url.query().find("cup2key=1:") != std::string::npos;
  }
};

TEST_F(BraveRequestSenderTest, DoesntUseBraveCUPKeyByDefault) {
  EXPECT_FALSE(UsesBraveCUPKey());
}

TEST_F(BraveRequestSenderTest, UsesBraveCUPKeyWhenOmaha4IsEnabled) {
  scoped_feature_list_.InitAndEnableFeature(brave::update_client::kBraveUseOmaha4);
  EXPECT_TRUE(UsesBraveCUPKey());
}

}  // namespace update_client

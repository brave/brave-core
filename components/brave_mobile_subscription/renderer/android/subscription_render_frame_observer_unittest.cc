/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_mobile_subscription/renderer/android/subscription_render_frame_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_subscription {

TEST(SubscriptionRenderFrameObserverTest, ExtractParam) {
  SubscriptionRenderFrameObserver observer(nullptr, 0);
  EXPECT_EQ(
      observer.ExtractParam(
          GURL("https://account.brave.com/?intent=connect-receipt&product=vpn"),
          "intent"),
      "connect-receipt");
  EXPECT_EQ(
      observer.ExtractParam(
          GURL("https://account.brave.com/"
               "?intent=connect-receipt1&product=vpn&intent=connect-receipt2"),
          "intent"),
      "connect-receipt1");
  EXPECT_EQ(
      observer.ExtractParam(
          GURL("https://account.brave.com/?intent=connect-receipt&product=vpn"),
          "product"),
      "vpn");
  EXPECT_EQ(
      observer.ExtractParam(
          GURL("https://account.brave.com/?intent=connect-receipt&product=leo"),
          "product"),
      "leo");
  EXPECT_TRUE(observer
                  .ExtractParam(GURL("https://account.brave.com/"
                                     "?intent=connect-receipt&product=vpn"),
                                "")
                  .empty());
  EXPECT_TRUE(observer
                  .ExtractParam(GURL("https://account.brave.com/"
                                     "?intent=connect-receipt&product=vpn"),
                                "somekey")
                  .empty());
  EXPECT_TRUE(
      observer.ExtractParam(GURL("https://account.brave.com/"), "intent")
          .empty());
  EXPECT_TRUE(observer.ExtractParam(GURL(""), "intent").empty());
}

TEST(SubscriptionRenderFrameObserverTest, IsValueAllowed) {
  SubscriptionRenderFrameObserver observer(nullptr, 0);

  EXPECT_FALSE(observer.IsValueAllowed(""));
  EXPECT_FALSE(observer.IsValueAllowed("alert(\"whoops\")"));
  EXPECT_TRUE(observer.IsValueAllowed("abc"));
  EXPECT_TRUE(observer.IsValueAllowed(
      "AO-J1OxJGS6-"
      "tNYvzofx7RO2hJSEgQmi6tOrLHEB4zJ2OhsyhX3mhEe4QKS0MVxtJCBNIAlBP5jAgDPqdXDN"
      "z15JhIXt5QYcIExIxe5H5ifbhAsHILlUXlE"));
}

}  // namespace brave_subscription

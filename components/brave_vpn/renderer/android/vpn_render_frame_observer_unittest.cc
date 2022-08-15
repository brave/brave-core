/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/renderer/android/vpn_render_frame_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_vpn {

TEST(VpnRenderFrameObserverTest, ExtractParam) {
  VpnRenderFrameObserver observer(nullptr, 0);
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

}  // namespace brave_vpn

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/peek/peek_utils.h"

#include "content/public/browser/page_navigator.h"
#include "content/public/common/referrer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace peek {

TEST(PeekUtilsTest, IsPeekableURL) {
  EXPECT_TRUE(IsPeekableURL(GURL("https://example.com/")));
  EXPECT_TRUE(IsPeekableURL(GURL("http://example.com/")));
  EXPECT_FALSE(IsPeekableURL(GURL("chrome://settings")));
  EXPECT_FALSE(IsPeekableURL(GURL("file:///tmp/a.html")));
  EXPECT_FALSE(IsPeekableURL(GURL("javascript:alert(1)")));
  EXPECT_FALSE(IsPeekableURL(GURL()));
}

TEST(PeekUtilsTest, IsShiftClickOpen) {
  const GURL url("https://example.org/");
  auto make_params = [&](WindowOpenDisposition disposition,
                         bool renderer_initiated) {
    content::OpenURLParams params(url, content::Referrer(), disposition,
                                  ui::PAGE_TRANSITION_LINK, renderer_initiated);
    params.user_gesture = true;
    return params;
  };

  EXPECT_TRUE(IsShiftClickOpen(make_params(WindowOpenDisposition::NEW_WINDOW,
                                           /*renderer_initiated=*/true)));
  // "Open link in new window" from the context menu is browser-initiated.
  EXPECT_FALSE(IsShiftClickOpen(make_params(WindowOpenDisposition::NEW_WINDOW,
                                            /*renderer_initiated=*/false)));
  EXPECT_FALSE(
      IsShiftClickOpen(make_params(WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                   /*renderer_initiated=*/true)));

  auto no_gesture = make_params(WindowOpenDisposition::NEW_WINDOW,
                                /*renderer_initiated=*/true);
  no_gesture.user_gesture = false;
  EXPECT_FALSE(IsShiftClickOpen(no_gesture));

  auto non_http = make_params(WindowOpenDisposition::NEW_WINDOW,
                              /*renderer_initiated=*/true);
  non_http.url = GURL("chrome://settings");
  EXPECT_FALSE(IsShiftClickOpen(non_http));
}

TEST(PeekUtilsTest, IsPeekableInitiator) {
  EXPECT_TRUE(IsPeekableInitiator(
      url::Origin::Create(GURL("https://example.com/page"))));
  EXPECT_TRUE(
      IsPeekableInitiator(url::Origin::Create(GURL("http://example.com/"))));
  EXPECT_FALSE(IsPeekableInitiator(url::Origin()));
  EXPECT_FALSE(
      IsPeekableInitiator(url::Origin::Create(GURL("chrome://settings"))));
  EXPECT_FALSE(
      IsPeekableInitiator(url::Origin::Create(GURL("file:///tmp/a.html"))));
  EXPECT_FALSE(
      IsPeekableInitiator(url::Origin::Create(GURL("data:text/html,hi"))));
}

TEST(PeekUtilsTest, CanOpenInPeek) {
  content::OpenURLParams params(GURL("https://b.com/"), content::Referrer(),
                                WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                ui::PAGE_TRANSITION_LINK,
                                /*is_renderer_initiated=*/true);
  // Without an initiator the load would look browser-initiated.
  EXPECT_FALSE(CanOpenInPeek(params));

  params.initiator_origin = url::Origin::Create(GURL("https://a.com/"));
  EXPECT_TRUE(CanOpenInPeek(params));

  params.initiator_origin = url::Origin();
  EXPECT_FALSE(CanOpenInPeek(params));

  params.initiator_origin = url::Origin::Create(GURL("https://a.com/"));
  params.url = GURL("data:text/html,hi");
  EXPECT_FALSE(CanOpenInPeek(params));
}

}  // namespace peek

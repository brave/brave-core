/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ai_chat_url_handler.h"
#include "content/public/browser/browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

TEST(AIChatContentBrowserClientHelper, HandleURLRewrite) {
  // Maps chrome://chat to chrome-untrusted://chat
  GURL url("chrome://chat");
  EXPECT_TRUE(HandleURLRewrite(&url, nullptr));
  EXPECT_EQ(url, GURL("chrome-untrusted://chat"));

  // chrome-untrusted://chat returns true so that URL reverse rewrite will be
  // hit but does not change the URL
  url = GURL("chrome-untrusted://chat");
  EXPECT_TRUE(HandleURLRewrite(&url, nullptr));
  EXPECT_EQ(url, GURL("chrome-untrusted://chat"));

  // Preserves the URL path, parameters, and fragments
  url = GURL("chrome://chat/test/test2?a=b&b=c#d");
  EXPECT_TRUE(HandleURLRewrite(&url, nullptr));
  EXPECT_EQ(url, GURL("chrome-untrusted://chat/test/test2?a=b&b=c#d"));
}

TEST(IPFSPortsTest, HandleURLReverseRewrite) {
  // Makes chrome-untrusted://chat loads show up as chrome://chat
  GURL url("chrome-untrusted://chat");
  EXPECT_TRUE(HandleURLReverseRewrite(&url, nullptr));
  EXPECT_EQ(url, GURL("chrome://chat"));

  // Preserves the URL path, parameters, and fragments
  url = GURL("chrome-untrusted://chat/test/test2?a=b&b=c#d");
  EXPECT_TRUE(HandleURLReverseRewrite(&url, nullptr));
  EXPECT_EQ(url, GURL("chrome://chat/test/test2?a=b&b=c#d"));
}

}  // namespace ai_chat

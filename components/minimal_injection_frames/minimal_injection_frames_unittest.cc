/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/minimal_injection_frames/minimal_injection_frames.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave {

namespace {

bool MatchesUrl(const char* spec) {
  return IsMinimalInjectionFrame(url::Origin::Create(GURL(spec)));
}

}  // namespace

TEST(MinimalInjectionFramesTest, MatchesListedOrigin) {
  EXPECT_TRUE(MatchesUrl("https://challenges.cloudflare.com"));
  EXPECT_TRUE(MatchesUrl("https://challenges.cloudflare.com/turnstile/v0.js"));
  // The default port and case are normalized away by url::Origin.
  EXPECT_TRUE(MatchesUrl("https://challenges.cloudflare.com:443/x"));
  EXPECT_TRUE(MatchesUrl("https://CHALLENGES.CLOUDFLARE.COM/x"));
  // Nested origins resolve to the inner origin.
  EXPECT_TRUE(MatchesUrl("blob:https://challenges.cloudflare.com/uuid"));
}

TEST(MinimalInjectionFramesTest, RequiresSameOrigin) {
  EXPECT_FALSE(MatchesUrl("http://challenges.cloudflare.com"));
  EXPECT_FALSE(MatchesUrl("https://challenges.cloudflare.com:8443"));
  EXPECT_FALSE(MatchesUrl("https://cloudflare.com"));
  EXPECT_FALSE(MatchesUrl("https://challenges.cloudflare.com.evil"));
  EXPECT_FALSE(MatchesUrl("https://evil.challenges.cloudflare.com"));
  // Host is evil.com, not the listed origin.
  EXPECT_FALSE(MatchesUrl("https://challenges.cloudflare.com@evil.com/x"));
}

TEST(MinimalInjectionFramesTest, DoesNotMatchOpaqueOrigins) {
  EXPECT_FALSE(IsMinimalInjectionFrame(url::Origin()));
  EXPECT_FALSE(MatchesUrl("about:blank"));
  EXPECT_FALSE(MatchesUrl("data:text/html,hi"));
  EXPECT_FALSE(MatchesUrl(""));
  EXPECT_FALSE(MatchesUrl("not a url"));
}

}  // namespace brave

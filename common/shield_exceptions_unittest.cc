/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/shield_exceptions.h"


#include "chrome/browser/devtools/url_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"

namespace {

typedef testing::Test BraveShieldsExceptionsTest;
using brave::IsWhitelistedReferrer;
using brave::IsBlockedResource;

TEST_F(BraveShieldsExceptionsTest, WidevineInstallableURL) {
  std::vector<GURL> urls({
    GURL("https://www.netflix.com/"),
    GURL("https://bitmovin.com/"),
    GURL("https://www.primevideo.com/"),
    GURL("https://www.spotify.com/"),
    GURL("https://shaka-player-demo.appspot.com"),
    GURL("https://www.netflix.com/subdir"),
    GURL("https://bitmovin.com/subdir"),
    GURL("https://www.primevideo.com/subdir"),
    GURL("https://www.spotify.com/subdir"),
    GURL("https://shaka-player-demo.appspot.com/subdir")
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    EXPECT_TRUE(brave::IsWidevineInstallableURL(url));
  });
}

TEST_F(BraveShieldsExceptionsTest, NotWidevineInstallableURL) {
  std::vector<GURL> urls({
    GURL("https://www.brave.com/"),
    GURL("https://widevine.com/")
  });
  std::for_each(urls.begin(), urls.end(),
      [this](GURL url){
    EXPECT_FALSE(brave::IsWidevineInstallableURL(url));
  });
}

TEST_F(BraveShieldsExceptionsTest, IsWhitelistedReferrer) {
  // *.fbcdn.net not allowed on some other URL
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://test.com"),
        GURL("https://video-zyz1-9.xy.fbcdn.net")));
  // *.fbcdn.net allowed on Facebook
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.facebook.com"),
        GURL("https://video-zyz1-9.xy.fbcdn.net")));
  // Facebook doesn't allow just anything
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.facebook.com.com"),
        GURL("https://test.com")));
  // Allowed for reddit.com
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
        GURL("https://www.redditmedia.com/97")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
        GURL("https://cdn.embedly.com/157")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
        GURL("https://imgur.com/179")));
  // Not allowed for reddit.com
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.reddit.com"),
        GURL("https://test.com")));
  // Not allowed imgur on another domain
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.test.com"),
        GURL("https://imgur.com/173")));
  // Fonts allowed anywhere
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.test.com"),
      GURL("https://use.typekit.net/193")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.test.com"),
      GURL("https://cloud.typography.com/199")));
}

TEST_F(BraveShieldsExceptionsTest, IsBlockedResource) {
  EXPECT_TRUE(IsBlockedResource(GURL("https://www.lesechos.fr/xtcore.js")));
  EXPECT_TRUE(IsBlockedResource(GURL("https://*.y8.com/js/sdkloader/outstream.js")));
  EXPECT_TRUE(IsBlockedResource(GURL(kRemoteFrontendBase)));
  EXPECT_TRUE(IsBlockedResource(GURL(std::string(kRemoteFrontendBase) + "137")));
  EXPECT_FALSE(IsBlockedResource(GURL("https://www.brave.com")));
}

}  // namespace

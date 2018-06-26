/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/shield_exceptions.h"


#include "chrome/test/base/chrome_render_view_host_test_harness.h"

namespace {

typedef testing::Test BraveShieldsExceptionsTest;

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

}  // namespace

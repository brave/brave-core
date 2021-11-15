/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"

#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "third_party/blink/public/common/features.h"

namespace {

struct BlinkMemoryCachePartitionTestCase {
  const std::string image_host;
  const std::vector<std::string> site_hosts;
  const int expected_image_requests_count;
};

}  // namespace

class EphemeralStorageBlinkMemoryCacheBrowserTest
    : public EphemeralStorageBrowserTest,
      public testing::WithParamInterface<BlinkMemoryCachePartitionTestCase> {
 public:
  EphemeralStorageBlinkMemoryCacheBrowserTest() {
    features_.InitAndEnableFeature(blink::features::kPartitionBlinkMemoryCache);
  }

  void NavigateAndWaitForImgLoad(Browser* browser,
                                 const GURL& url,
                                 const GURL& img_url) {
    constexpr char kLoadImgAsync[] = R"(
      (async () => {
        let img = document.createElement("img");
        document.body.appendChild(img);
        let imgLoadPromise = new Promise((resolve, reject) => {
          img.addEventListener("load", resolve, {once: true});
        });
        img.src = '%s';
        await imgLoadPromise;
      })();
    )";

    auto* rfh = ui_test_utils::NavigateToURLWithDisposition(
        browser, url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    EXPECT_TRUE(content::ExecJs(
        content::ChildFrameAt(rfh, 0),
        base::StringPrintf(kLoadImgAsync, img_url.spec().c_str())));
  }

 private:
  base::test::ScopedFeatureList features_;
};

IN_PROC_BROWSER_TEST_P(EphemeralStorageBlinkMemoryCacheBrowserTest,
                       BlinkMemoryCacheIsUsedProperly) {
  const auto& test_case = GetParam();
  for (Browser* browser_instance : {browser(), CreateIncognitoBrowser()}) {
    const GURL image_url =
        https_server_.GetURL(test_case.image_host, "/logo.png?cache");

    for (const auto& site_host : test_case.site_hosts) {
      const GURL site_url =
          https_server_.GetURL(site_host, "/ephemeral_storage.html");
      NavigateAndWaitForImgLoad(browser_instance, site_url, image_url);
    }

    EXPECT_EQ(http_request_monitor_.GetHttpRequestsCount(image_url),
              test_case.expected_image_requests_count);
    http_request_monitor_.Clear();
  }
}

INSTANTIATE_TEST_SUITE_P(,
                         EphemeralStorageBlinkMemoryCacheBrowserTest,
                         ::testing::Values(
                             BlinkMemoryCachePartitionTestCase{
                                 "b.com",
                                 // Different sites.
                                 {"a.com", "c.com", "d.com"},
                                 3,
                             },
                             BlinkMemoryCachePartitionTestCase{
                                 "b.com",
                                 // Different sites and subsite.
                                 {"a.com", "c.com", "sub.a.com"},
                                 2,
                             },
                             BlinkMemoryCachePartitionTestCase{
                                 "b.com",
                                 // Same site with subsite.
                                 {"b.com", "sub.b.com"},
                                 1,
                             },
                             BlinkMemoryCachePartitionTestCase{
                                 "github.io",
                                 // PSL-matched different sites.
                                 {"github.io", "user.github.io",
                                  "dev.github.io"},
                                 3,
                             }));

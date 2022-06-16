/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/pattern.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_frame_navigation_observer.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/base/features.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

constexpr char kCreateBlobScript[] = "URL.createObjectURL(new Blob([$1]))";

constexpr char kFetchBlobScript[] = R"(
(async function() {
  try {
    const response = await fetch($1);
    return await response.text();
  } catch (err) {
    return 'error';
  }
})();)";

constexpr char kWorkerScript[] = R"(
  (async() => {
    try {
      const response = await fetch($1);
      postMessage(await response.text());
    } catch (err) {
      postMessage('error');
    }
  })();
)";

constexpr char kFetchBlobViaWorkerScript[] = R"(
  new Promise(async (resolve) => {
    const blobURL = URL.createObjectURL(new Blob([$1]));
    const dedicatedWorker = new Worker(blobURL);
    dedicatedWorker.addEventListener('message', e => {
      resolve(e.data);
    });
  });
)";

}  // namespace

class BlobUrlBrowserTestBase : public EphemeralStorageBrowserTest {
 public:
  struct RenderFrameHostBlobData {
    content::RenderFrameHost* rfh;
    GURL blob_url;
  };
  using FramesWithRegisteredBlobs = std::vector<RenderFrameHostBlobData>;

  void SetUpOnMainThread() override {
    EphemeralStorageBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  static GURL RegisterBlob(content::RenderFrameHost* render_frame_host,
                           base::StringPiece content) {
    return GURL(EvalJs(render_frame_host,
                       content::JsReplace(kCreateBlobScript, content))
                    .ExtractString());
  }

  static content::EvalJsResult FetchBlob(
      content::RenderFrameHost* render_frame_host,
      const GURL& url) {
    auto fetch_result = EvalJs(
        render_frame_host, content::JsReplace(kFetchBlobScript, url.spec()));
    auto fetch_via_webworker_result = EvalJs(
        render_frame_host,
        content::JsReplace(kFetchBlobViaWorkerScript,
                           content::JsReplace(kWorkerScript, url.spec())));
    EXPECT_EQ(fetch_result.value, fetch_via_webworker_result.value);
    EXPECT_EQ(fetch_result.error, fetch_via_webworker_result.error);
    return fetch_result;
  }

  static void NavigateToBlob(content::RenderFrameHost* render_frame_host,
                             const GURL& url) {
    std::string script = content::JsReplace("location = $1", url.spec());
    content::TestFrameNavigationObserver observer(render_frame_host);
    EXPECT_TRUE(ExecJs(render_frame_host, script));
    observer.Wait();
    EXPECT_EQ(url, render_frame_host->GetLastCommittedURL());
    EXPECT_FALSE(render_frame_host->IsErrorDocument());
  }

  static void EnsureBlobsAreCrossAvailable(
      const FramesWithRegisteredBlobs& frames_with_registered_blobs,
      size_t rfh1_idx,
      size_t rfh2_idx) {
    EXPECT_EQ(std::to_string(rfh2_idx),
              FetchBlob(frames_with_registered_blobs[rfh1_idx].rfh,
                        frames_with_registered_blobs[rfh2_idx].blob_url));
    EXPECT_EQ(std::to_string(rfh1_idx),
              FetchBlob(frames_with_registered_blobs[rfh2_idx].rfh,
                        frames_with_registered_blobs[rfh1_idx].blob_url));
  }

  static std::vector<content::RenderFrameHost*> GetFrames(
      content::RenderFrameHost* main_frame) {
    return {
        main_frame,
        content::ChildFrameAt(main_frame, 0),
        content::ChildFrameAt(main_frame, 1),
        content::ChildFrameAt(main_frame, 2),
        content::ChildFrameAt(main_frame, 3),
    };
  }

  FramesWithRegisteredBlobs RegisterBlobs(const GURL& url) {
    FramesWithRegisteredBlobs frames_with_registered_blobs;
    content::RenderFrameHost* main_frame = LoadURLInNewTab(url)->GetMainFrame();
    const std::vector<content::RenderFrameHost*> rfhs = GetFrames(main_frame);

    for (size_t idx = 0; idx < rfhs.size(); ++idx) {
      auto* rfh = rfhs[idx];
      // Register blob.
      GURL blob_url = RegisterBlob(rfh, std::to_string(idx));
      // Blob should be fetchable from the same frame.
      EXPECT_EQ(std::to_string(idx), FetchBlob(rfh, blob_url));

      frames_with_registered_blobs.push_back({rfh, std::move(blob_url)});
    }
    return frames_with_registered_blobs;
  }

  void TestBlobsArePartitioned() {
    // Register blobs in a.com and its subframes, check blobs can be fetched
    // from
    // originating frames without any issues.
    FramesWithRegisteredBlobs a_com_registered_blobs =
        RegisterBlobs(a_site_ephemeral_storage_url_);
    // Expect blob created from a.com is available in iframe and vice versa.
    EnsureBlobsAreCrossAvailable(a_com_registered_blobs, 0, 3);
    // Expect blob created from b.com iframe is available in another b.com
    // iframe and vice versa.
    EnsureBlobsAreCrossAvailable(a_com_registered_blobs, 1, 2);

    // Register blobs in b.com and its subframes, check they can be fetched from
    // originating frames without any issues.
    FramesWithRegisteredBlobs b_com_registered_blobs =
        RegisterBlobs(b_site_ephemeral_storage_url_);

    // Ensure no blobs from a.com are available to fetch in b.com iframes.
    for (size_t idx = 0; idx < b_com_registered_blobs.size(); ++idx) {
      auto* rfh = b_com_registered_blobs[idx].rfh;
      // No blobs from a.com should be available.
      for (size_t a_com_blob_idx = 0;
           a_com_blob_idx < a_com_registered_blobs.size(); ++a_com_blob_idx) {
        EXPECT_EQ(
            "error",
            FetchBlob(rfh, a_com_registered_blobs[a_com_blob_idx].blob_url));
      }
    }

    // Expect all a.com blobs (including the ones from 3p frames) are
    // available in another a.com tab.
    FramesWithRegisteredBlobs a_com2_registered_blobs =
        RegisterBlobs(a_site_ephemeral_storage_url_);
    for (size_t idx = 0; idx < a_com2_registered_blobs.size(); ++idx) {
      auto* rfh = a_com2_registered_blobs[idx].rfh;
      // All blobs from another a.com tab should be avilable.
      EXPECT_EQ(std::to_string(idx),
                FetchBlob(rfh, a_com_registered_blobs[idx].blob_url));

      // No blobs from b.com should be available.
      for (size_t b_com_blob_idx = 0;
           b_com_blob_idx < b_com_registered_blobs.size(); ++b_com_blob_idx) {
        EXPECT_EQ(
            "error",
            FetchBlob(rfh, b_com_registered_blobs[b_com_blob_idx].blob_url));
      }
    }

    // Close the first a.com tab, ensure all blobs created there become obsolete
    // and can't be fetched.
    ASSERT_TRUE(browser()->tab_strip_model()->CloseWebContentsAt(
        1, TabStripModel::CloseTypes::CLOSE_NONE));
    content::RunAllTasksUntilIdle();
    for (size_t idx = 0; idx < a_com2_registered_blobs.size(); ++idx) {
      auto* rfh = a_com2_registered_blobs[idx].rfh;
      EXPECT_EQ("error", FetchBlob(rfh, a_com_registered_blobs[idx].blob_url));
    }

    // Ensure blobs are navigatable in same iframes.
    for (size_t idx = 1; idx < a_com2_registered_blobs.size(); ++idx) {
      NavigateToBlob(a_com2_registered_blobs[idx].rfh,
                     a_com2_registered_blobs[idx].blob_url);
    }
    for (size_t idx = 1; idx < b_com_registered_blobs.size(); ++idx) {
      NavigateToBlob(b_com_registered_blobs[idx].rfh,
                     b_com_registered_blobs[idx].blob_url);
    }
  }

  void TestBlobsAreNotPartitioned() {
    // Register blobs in a.com and its subframes, check blobs can be fetched
    // from
    // originating frames without any issues.
    FramesWithRegisteredBlobs a_com_registered_blobs =
        RegisterBlobs(a_site_ephemeral_storage_url_);
    // Expect blob created from a.com is available in iframe and vice versa.
    EnsureBlobsAreCrossAvailable(a_com_registered_blobs, 0, 3);
    // Expect blob created from b.com iframe is available in another b.com
    // iframe and vice versa.
    EnsureBlobsAreCrossAvailable(a_com_registered_blobs, 1, 2);

    // Register blobs in b.com and its subframes, check they can be fetched from
    // originating frames without any issues.
    FramesWithRegisteredBlobs b_com_registered_blobs =
        RegisterBlobs(b_site_ephemeral_storage_url_);

    // Ensure blobs from a.com tab are available to fetch in b.com tab.
    for (size_t idx = 0; idx < b_com_registered_blobs.size(); ++idx) {
      auto* rfh = b_com_registered_blobs[idx].rfh;
      // Blobs from a.com should be available.
      if (idx == 0) {
        const size_t b_com_inside_a_com_idx = 1;
        EXPECT_EQ(
            std::to_string(b_com_inside_a_com_idx),
            FetchBlob(rfh,
                      a_com_registered_blobs[b_com_inside_a_com_idx].blob_url));
      } else {
        EXPECT_EQ(std::to_string(idx),
                  FetchBlob(rfh, a_com_registered_blobs[idx].blob_url));
      }
    }

    // Expect all a.com blobs (including the ones from 3p frames) are
    // available in another a.com tab.
    FramesWithRegisteredBlobs a_com2_registered_blobs =
        RegisterBlobs(a_site_ephemeral_storage_url_);
    for (size_t idx = 0; idx < a_com2_registered_blobs.size(); ++idx) {
      auto* rfh = a_com2_registered_blobs[idx].rfh;
      // All blobs from another a.com tab should be avilable.
      EXPECT_EQ(std::to_string(idx),
                FetchBlob(rfh, a_com_registered_blobs[idx].blob_url));

      // Blobs from b.com should also be available.
      if (idx == 0) {
        const size_t a_com_inside_b_com_idx = 3;
        EXPECT_EQ(
            std::to_string(a_com_inside_b_com_idx),
            FetchBlob(rfh,
                      a_com_registered_blobs[a_com_inside_b_com_idx].blob_url));
      } else {
        EXPECT_EQ(std::to_string(idx),
                  FetchBlob(rfh, b_com_registered_blobs[idx].blob_url));
      }
    }

    // Close the first a.com tab, ensure all blobs created there become obsolete
    // and can't be fetched.
    ASSERT_TRUE(browser()->tab_strip_model()->CloseWebContentsAt(
        1, TabStripModel::CloseTypes::CLOSE_NONE));
    content::RunAllTasksUntilIdle();
    for (size_t idx = 0; idx < a_com2_registered_blobs.size(); ++idx) {
      auto* rfh = a_com2_registered_blobs[idx].rfh;
      EXPECT_EQ("error", FetchBlob(rfh, a_com_registered_blobs[idx].blob_url));
    }

    // Ensure blobs are navigatable in same iframes.
    for (size_t idx = 1; idx < a_com2_registered_blobs.size(); ++idx) {
      NavigateToBlob(a_com2_registered_blobs[idx].rfh,
                     a_com2_registered_blobs[idx].blob_url);
    }
    for (size_t idx = 1; idx < b_com_registered_blobs.size(); ++idx) {
      NavigateToBlob(b_com_registered_blobs[idx].rfh,
                     b_com_registered_blobs[idx].blob_url);
    }
  }
};

class BlobUrlPartitionEnabledBrowserTest : public BlobUrlBrowserTestBase {
 public:
  BlobUrlPartitionEnabledBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        net::features::kBravePartitionBlobStorage);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledBrowserTest,
                       BlobsArePartitioned) {
  TestBlobsArePartitioned();
}

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledBrowserTest,
                       BlobsAreNotPartitionedWhenShieldsDisabled) {
  // Disable shields on a.com and b.com.
  brave_shields::SetBraveShieldsEnabled(content_settings(), false,
                                        a_site_ephemeral_storage_url_);
  brave_shields::SetBraveShieldsEnabled(content_settings(), false,
                                        b_site_ephemeral_storage_url_);

  TestBlobsAreNotPartitioned();
}

class BlobUrlPartitionEnabledWith1PESBrowserTest
    : public BlobUrlBrowserTestBase {
 public:
  BlobUrlPartitionEnabledWith1PESBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {net::features::kBraveFirstPartyEphemeralStorage,
         net::features::kBravePartitionBlobStorage},
        {});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledWith1PESBrowserTest,
                       BlobsArePartitionedIn1PESMode) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  TestBlobsArePartitioned();
}

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledWith1PESBrowserTest,
                       BlobsArePartitionedIn1PESModeForBothSites) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  SetCookieSetting(b_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  TestBlobsArePartitioned();
}

class BlobUrlPartitionDisabledBrowserTest : public BlobUrlBrowserTestBase {
 public:
  BlobUrlPartitionDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(
        net::features::kBravePartitionBlobStorage);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionDisabledBrowserTest,
                       BlobsAreNotPartitioned) {
  TestBlobsAreNotPartitioned();
}

class BlobUrlEphemeralStorageDisabledBrowserTest
    : public BlobUrlBrowserTestBase {
 public:
  BlobUrlEphemeralStorageDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(
        net::features::kBraveEphemeralStorage);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BlobUrlEphemeralStorageDisabledBrowserTest,
                       BlobsAreNotPartitioned) {
  TestBlobsAreNotPartitioned();
}

// Tests for the blob: URL scheme, originally implemented in
// content/browser/blob_storage/blob_url_browsertest.cc,
// migrated from content_browsertests to brave_browser_tests.
class BlobUrlBrowserTest : public BlobUrlBrowserTestBase,
                           public testing::WithParamInterface<bool> {
 public:
  BlobUrlBrowserTest() {
    scoped_feature_list_.InitWithFeatureState(
        net::features::kBravePartitionBlobStorage, GetParam());
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

INSTANTIATE_TEST_SUITE_P(, BlobUrlBrowserTest, testing::Bool());

IN_PROC_BROWSER_TEST_P(BlobUrlBrowserTest, LinkToUniqueOriginBlob) {
  // Use a data URL to obtain a test page in a unique origin. The page
  // contains a link to a "blob:null/SOME-GUID-STRING" URL.
  auto* rfh = ui_test_utils::NavigateToURL(
      browser(),
      GURL("data:text/html,<body><script>"
           "var link = document.body.appendChild(document.createElement('a'));"
           "link.innerText = 'Click Me!';"
           "link.href = URL.createObjectURL(new Blob(['potato']));"
           "link.target = '_blank';"
           "link.id = 'click_me';"
           "</script></body>"));
  ASSERT_TRUE(rfh);

  // Click the link.
  content::WebContentsAddedObserver window_observer;
  EXPECT_TRUE(ExecJs(rfh, "document.getElementById('click_me').click()"));
  content::WebContents* new_contents = window_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(new_contents));

  EXPECT_TRUE(
      base::MatchPattern(new_contents->GetVisibleURL().spec(), "blob:null/*"));
  EXPECT_EQ(
      "null potato",
      EvalJs(new_contents, "self.origin + ' ' + document.body.innerText;"));
}

IN_PROC_BROWSER_TEST_P(BlobUrlBrowserTest, LinkToSameOriginBlob) {
  // Using an http page, click a link that opens a popup to a same-origin blob.
  GURL url = https_server_.GetURL("chromium.org", "/title1.html");
  url::Origin origin = url::Origin::Create(url);
  auto* rfh = ui_test_utils::NavigateToURL(browser(), url);

  content::WebContentsAddedObserver window_observer;
  EXPECT_TRUE(ExecJs(
      rfh,
      "var link = document.body.appendChild(document.createElement('a'));"
      "link.innerText = 'Click Me!';"
      "link.href = URL.createObjectURL(new Blob(['potato']));"
      "link.target = '_blank';"
      "link.click()"));

  // The link should create a new tab.
  content::WebContents* new_contents = window_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(new_contents));

  EXPECT_TRUE(base::MatchPattern(new_contents->GetVisibleURL().spec(),
                                 "blob:" + origin.Serialize() + "/*"));
  EXPECT_EQ(
      origin.Serialize() + " potato",
      EvalJs(new_contents, "    self.origin + ' ' + document.body.innerText;"));
}

// Regression test for https://crbug.com/646278
IN_PROC_BROWSER_TEST_P(BlobUrlBrowserTest, LinkToSameOriginBlobWithAuthority) {
  // Using an http page, click a link that opens a popup to a same-origin blob
  // that has a spoofy authority section applied. This should be blocked.
  GURL url = embedded_test_server()->GetURL("chromium.org", "/title1.html");
  url::Origin origin = url::Origin::Create(url);
  auto* rfh = ui_test_utils::NavigateToURL(browser(), url);

  content::WebContentsAddedObserver window_observer;
  EXPECT_TRUE(ExecJs(
      rfh,
      "var link = document.body.appendChild(document.createElement('a'));"
      "link.innerText = 'Click Me!';"
      "link.href = 'blob:http://spoof.com@' + "
      "    URL.createObjectURL(new Blob(['potato'])).split('://')[1];"
      "link.rel = 'opener'; link.target = '_blank';"
      "link.click()"));

  // The link should create a new tab.
  content::WebContents* new_contents = window_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(new_contents));

  // The spoofy URL should not be shown to the user.
  EXPECT_FALSE(
      base::MatchPattern(new_contents->GetVisibleURL().spec(), "*spoof*"));
  // The currently implemented behavior is that the URL gets rewritten to
  // about:blank#blocked.
  EXPECT_EQ(content::kBlockedURL, new_contents->GetVisibleURL().spec());
  EXPECT_EQ(
      origin.Serialize() + " ",
      EvalJs(new_contents,
             "self.origin + ' ' + document.body.innerText;"));  // no potato
}

// Regression test for https://crbug.com/646278
IN_PROC_BROWSER_TEST_P(BlobUrlBrowserTest, ReplaceStateToAddAuthorityToBlob) {
  // history.replaceState from a validly loaded blob URL shouldn't allow adding
  // an authority to the inner URL, which would be spoofy.
  GURL url = embedded_test_server()->GetURL("chromium.org", "/title1.html");
  url::Origin origin = url::Origin::Create(url);
  auto* rfh = ui_test_utils::NavigateToURL(browser(), url);

  content::WebContentsAddedObserver window_observer;
  EXPECT_TRUE(ExecJs(
      rfh,
      "var spoof_fn = function () {\n"
      "  host_port = self.origin.split('://')[1];\n"
      "  spoof_url = 'blob:http://spoof.com@' + host_port + '/abcd';\n"
      "  window.history.replaceState({}, '', spoof_url);\n"
      "};\n"
      "args = ['<body>potato<scr', 'ipt>(', spoof_fn, ')();</scri', 'pt>'];\n"
      "b = new Blob(args, {type: 'text/html'});"
      "window.open(URL.createObjectURL(b));"));

  content::WebContents* new_contents = window_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(new_contents));

  // The spoofy URL should not be shown to the user.
  EXPECT_FALSE(
      base::MatchPattern(new_contents->GetVisibleURL().spec(), "*spoof*"));

  // The currently implemented behavior is that the URL gets rewritten to
  // about:blank#blocked. The content of the page stays the same.
  EXPECT_EQ(content::kBlockedURL, new_contents->GetVisibleURL().spec());
  EXPECT_EQ(
      origin.Serialize() + " potato",
      EvalJs(new_contents, "self.origin + ' ' + document.body.innerText;"));

  // TODO(nick): Currently, window.location still reflects the spoof URL.
  // This seems unfortunate -- can we fix it?
  std::string window_location =
      EvalJs(new_contents, "window.location.href;").ExtractString();
  EXPECT_TRUE(base::MatchPattern(window_location, "*spoof*"));
}

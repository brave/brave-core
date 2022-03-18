/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/pattern.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
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

class BlobUrlBrowserTest : public EphemeralStorageBrowserTest {
 public:
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
};

IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, BlobsArePartitioned) {
  // Register blobs in a.com and its subframes, check blobs can be fetched from
  // originating frames without any issues.
  std::vector<GURL> a_com_registered_blobs;
  {
    content::RenderFrameHost* main_frame =
        LoadURLInNewTab(a_site_ephemeral_storage_url_)->GetMainFrame();
    const std::vector<content::RenderFrameHost*> rfhs = GetFrames(main_frame);

    for (size_t idx = 0; idx < rfhs.size(); ++idx) {
      auto* rfh = rfhs[idx];
      // Register blob.
      a_com_registered_blobs.push_back(RegisterBlob(rfh, std::to_string(idx)));

      // Blob should be fetchable from the same frame.
      EXPECT_EQ(std::to_string(idx),
                FetchBlob(rfh, a_com_registered_blobs[idx]));
    }

    // Expect blob created from a.com is available in iframe and vice versa.
    EXPECT_EQ(std::to_string(0), FetchBlob(rfhs[3], a_com_registered_blobs[0]));
    EXPECT_EQ(std::to_string(3), FetchBlob(rfhs[0], a_com_registered_blobs[3]));

    // Expect blob created from b.com iframe is available in another b.com
    // iframe and vice versa.
    EXPECT_EQ(std::to_string(1), FetchBlob(rfhs[2], a_com_registered_blobs[1]));
    EXPECT_EQ(std::to_string(2), FetchBlob(rfhs[1], a_com_registered_blobs[2]));
  }

  // Register blobs in b.com and its subframes, check they can be fetched from
  // originating frames without any issues.
  // Ensure no blobs from a.com are available to fetch in iframes.
  std::vector<GURL> b_com_registered_blobs;
  {
    content::RenderFrameHost* main_frame =
        LoadURLInNewTab(b_site_ephemeral_storage_url_)->GetMainFrame();
    const std::vector<content::RenderFrameHost*> rfhs = GetFrames(main_frame);

    for (size_t idx = 0; idx < rfhs.size(); ++idx) {
      auto* rfh = rfhs[idx];
      // Register blob.
      b_com_registered_blobs.push_back(RegisterBlob(rfh, std::to_string(idx)));

      // Blob should be fetchable from the same frame.
      EXPECT_EQ(std::to_string(idx),
                FetchBlob(rfh, b_com_registered_blobs[idx]));

      // No blobs from a.com should be available.
      for (size_t a_com_blob_idx = 0;
           a_com_blob_idx < a_com_registered_blobs.size(); ++a_com_blob_idx) {
        EXPECT_EQ("error",
                  FetchBlob(rfh, a_com_registered_blobs[a_com_blob_idx]));
      }
    }
  }

  // Expect all a.com blobs (including the ones from 3p frames) are
  // available in another a.com tab.
  {
    content::RenderFrameHost* main_frame =
        LoadURLInNewTab(a_site_ephemeral_storage_url_)->GetMainFrame();
    const std::vector<content::RenderFrameHost*> rfhs = GetFrames(main_frame);

    for (size_t idx = 0; idx < rfhs.size(); ++idx) {
      auto* rfh = rfhs[idx];
      // All blobs from another a.com tab should be avilable.
      EXPECT_EQ(std::to_string(idx),
                FetchBlob(rfh, a_com_registered_blobs[idx]));

      // No blobs from b.com should be available.
      for (size_t b_com_blob_idx = 0;
           b_com_blob_idx < b_com_registered_blobs.size(); ++b_com_blob_idx) {
        EXPECT_EQ("error",
                  FetchBlob(rfh, b_com_registered_blobs[b_com_blob_idx]));
      }
    }

    // Close the first a.com tab, ensure all blobs created there become obsolete
    // and can't be fetched.
    ASSERT_TRUE(browser()->tab_strip_model()->CloseWebContentsAt(
        1, TabStripModel::CloseTypes::CLOSE_NONE));
    content::RunAllTasksUntilIdle();
    for (size_t idx = 0; idx < rfhs.size(); ++idx) {
      auto* rfh = rfhs[idx];
      EXPECT_EQ("error", FetchBlob(rfh, a_com_registered_blobs[idx]));
    }
  }
}

// Tests for the blob: URL scheme, originally implemented in
// content/browser/blob_storage/blob_url_browsertest.cc,
// migrated from content_browsertests to brave_browser_tests.
using BlobUrlBrowserTest_Chromium = BlobUrlBrowserTest;

IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest_Chromium, LinkToUniqueOriginBlob) {
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

IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest_Chromium, LinkToSameOriginBlob) {
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
IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest_Chromium,
                       LinkToSameOriginBlobWithAuthority) {
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
IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest_Chromium,
                       ReplaceStateToAddAuthorityToBlob) {
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

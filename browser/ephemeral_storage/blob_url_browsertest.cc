/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/strings/pattern.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_browsertest.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_frame_navigation_observer.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/features.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "extensions/test/test_extension_dir.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

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
    raw_ptr<content::RenderFrameHost> rfh;
    GURL blob_url;
  };
  using FramesWithRegisteredBlobs = std::vector<RenderFrameHostBlobData>;

  static GURL RegisterBlob(content::RenderFrameHost* render_frame_host,
                           std::string_view content) {
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
    EXPECT_EQ(url, observer.last_committed_url());
    EXPECT_TRUE(observer.last_navigation_succeeded());
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
    content::RenderFrameHost* main_frame =
        LoadURLInNewTab(url)->GetPrimaryMainFrame();
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
    for (auto& b_com_registered_blob : b_com_registered_blobs) {
      auto* rfh = b_com_registered_blob.rfh.get();
      // No blobs from a.com should be available.
      for (auto& a_com_registered_blob : a_com_registered_blobs) {
        EXPECT_EQ("error", FetchBlob(rfh, a_com_registered_blob.blob_url));
      }
    }

    // Expect all a.com blobs (including the ones from 3p frames) are
    // available in another a.com tab.
    FramesWithRegisteredBlobs a_com2_registered_blobs =
        RegisterBlobs(a_site_ephemeral_storage_url_);
    for (size_t idx = 0; idx < a_com2_registered_blobs.size(); ++idx) {
      auto* rfh = a_com2_registered_blobs[idx].rfh.get();
      // All blobs from another a.com tab should be avilable.
      EXPECT_EQ(std::to_string(idx),
                FetchBlob(rfh, a_com_registered_blobs[idx].blob_url));

      // No blobs from b.com should be available.
      for (auto& b_com_registered_blob : b_com_registered_blobs) {
        EXPECT_EQ("error", FetchBlob(rfh, b_com_registered_blob.blob_url));
      }
    }

    // Close the first a.com tab, ensure all blobs created there become obsolete
    // and can't be fetched.
    const int previous_tab_count = browser()->tab_strip_model()->count();
    browser()->tab_strip_model()->CloseWebContentsAt(1,
                                                     TabCloseTypes::CLOSE_NONE);
    EXPECT_EQ(previous_tab_count - 1, browser()->tab_strip_model()->count());
    content::RunAllTasksUntilIdle();
    for (size_t idx = 0; idx < a_com2_registered_blobs.size(); ++idx) {
      auto* rfh = a_com2_registered_blobs[idx].rfh.get();
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

using BlobUrlPartitionEnabledBrowserTest = BlobUrlBrowserTestBase;

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledBrowserTest,
                       BlobsArePartitioned) {
  TestBlobsArePartitioned();
}

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledBrowserTest,
                       BlobsWithFragmentAreAccessible) {
  FramesWithRegisteredBlobs a_com_registered_blobs =
      RegisterBlobs(a_site_ephemeral_storage_url_);

  for (auto& registered_blob : a_com_registered_blobs) {
    registered_blob.blob_url = GURL(registered_blob.blob_url.spec() + "#test");
  }

  // Expect blob created from a.com is available in iframe and vice versa.
  EnsureBlobsAreCrossAvailable(a_com_registered_blobs, 0, 3);
  // Expect blob created from b.com iframe is available in another b.com
  // iframe and vice versa.
  EnsureBlobsAreCrossAvailable(a_com_registered_blobs, 1, 2);
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledBrowserTest,
                       BlobsAreAccessibleFromExtension) {
  FramesWithRegisteredBlobs a_com_registered_blobs =
      RegisterBlobs(a_site_ephemeral_storage_url_);

  extensions::TestExtensionDir test_extension_dir;
  test_extension_dir.WriteManifest(R"({
    "name": "Test",
    "manifest_version": 2,
    "version": "0.1",
    "permissions": ["webRequest", "*://a.com/*", "*://b.com/*"],
    "content_security_policy":
      "script-src 'self' 'unsafe-eval'; object-src 'self'"
  })");
  test_extension_dir.WriteFile(FILE_PATH_LITERAL("empty.html"), "");

  extensions::ChromeTestExtensionLoader extension_loader(browser()->profile());
  scoped_refptr<const extensions::Extension> extension =
      extension_loader.LoadExtension(test_extension_dir.UnpackedPath());
  const GURL url = extension->GetResourceURL("/empty.html");
  auto* extension_rfh = ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  ASSERT_TRUE(extension_rfh);

  for (size_t idx = 0; idx < a_com_registered_blobs.size(); ++idx) {
    SCOPED_TRACE(a_com_registered_blobs[idx].blob_url.spec());
    EXPECT_EQ(base::NumberToString(idx),
              FetchBlob(extension_rfh, a_com_registered_blobs[idx].blob_url));
  }
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

class BlobUrlPartitionEnabledWithoutSiteIsolationBrowserTest
    : public BlobUrlPartitionEnabledBrowserTest {
 public:
  BlobUrlPartitionEnabledWithoutSiteIsolationBrowserTest() = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    BlobUrlPartitionEnabledBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kDisableSiteIsolation);
  }
};

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledWithoutSiteIsolationBrowserTest,
                       BlobsArePartitioned) {
  TestBlobsArePartitioned();
}

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledBrowserTest,
                       BlobsArePartitionedIn1PESMode) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  TestBlobsArePartitioned();
}

IN_PROC_BROWSER_TEST_F(BlobUrlPartitionEnabledBrowserTest,
                       BlobsArePartitionedIn1PESModeForBothSites) {
  SetCookieSetting(a_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  SetCookieSetting(b_site_ephemeral_storage_url_, CONTENT_SETTING_SESSION_ONLY);
  TestBlobsArePartitioned();
}

// Tests for the blob: URL scheme, originally implemented in
// content/browser/blob_storage/blob_url_browsertest.cc,
// migrated from content_browsertests to brave_browser_tests.
class BlobUrlBrowserTest : public BlobUrlBrowserTestBase,
                           public testing::WithParamInterface<bool> {
 public:
  BlobUrlBrowserTest() = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, LinkToUniqueOriginBlob) {
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

IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, LinkToSameOriginBlob) {
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
IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, LinkToSameOriginBlobWithAuthority) {
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
IN_PROC_BROWSER_TEST_F(BlobUrlBrowserTest, ReplaceStateToAddAuthorityToBlob) {
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

  EXPECT_EQ(
      origin.Serialize() + " potato",
      EvalJs(new_contents, "self.origin + ' ' + document.body.innerText;"));

  // TODO(nick): Currently, window.location still reflects the spoof URL.
  // This seems unfortunate -- can we fix it?
  std::string window_location =
      EvalJs(new_contents, "window.location.href;").ExtractString();
  EXPECT_FALSE(base::MatchPattern(window_location, "*spoof*"));
}

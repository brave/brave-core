/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/browser/ui/webui/brave_wallet/snap_host/snap_host_ui.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/webui_config_map.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace brave_wallet {

class SnapHostBrowserTest : public InProcessBrowserTest {
 public:
  SnapHostBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveWalletSnapFeature);
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    ASSERT_TRUE(https_server_.Start());
  }

  content::RenderFrameHost* AppendSubframe(content::RenderFrameHost* frame,
                                           const GURL& url,
                                           const std::string& sandbox) {
    EXPECT_TRUE(content::ExecJs(frame,
                                content::JsReplace(R"js(
        new Promise(resolve => {
          const iframe = document.createElement('iframe');
          iframe.onload = resolve;
          iframe.setAttribute('sandbox', $2);
          iframe.src = $1;
          document.body.appendChild(iframe);
        })
      )js",
                                                   url, sandbox),
                                content::EXECUTE_SCRIPT_NO_USER_GESTURE));
    return content::ChildFrameAt(frame, 0);
  }

  content::RenderFrameHost* AppendSnapHostFrame(
      content::RenderFrameHost* frame) {
    return AppendSubframe(frame, GURL(kUntrustedSnapURL),
                          "allow-scripts allow-same-origin");
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

class SnapHostFeatureDisabledBrowserTest : public InProcessBrowserTest {
 public:
  SnapHostFeatureDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kBraveWalletSnapFeature);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Proves resource wiring: the host loads and signals readiness to its
// parent.
IN_PROC_BROWSER_TEST_F(SnapHostBrowserTest, HostResourcesLoadAndSignalReady) {
  auto* host_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUISnapsContainerURL));
  ASSERT_TRUE(host_rfh);

  // The assignment's value is the pending Promise itself, so this must not
  // wait on it — it only resolves once the iframe below is created.
  ASSERT_TRUE(content::ExecJs(host_rfh, R"js(
    window.executorReadyPromise = new Promise(resolve => {
      window.addEventListener('message', function onMsg(event) {
        if (event.data && event.data.type === 'executorReady') {
          window.removeEventListener('message', onMsg);
          resolve(true);
        }
      });
    });
  )js",
                              content::EXECUTE_SCRIPT_NO_RESOLVE_PROMISES));

  auto* snap_rfh = AppendSnapHostFrame(host_rfh);
  ASSERT_TRUE(snap_rfh);
  EXPECT_FALSE(snap_rfh->IsErrorDocument());

  EXPECT_EQ(true, content::EvalJs(host_rfh, "window.executorReadyPromise"));
}

// Proves the new Function() CJS wrapper: a CommonJS bundle evaluates and
// reports executeSnapResult with the exported string.
IN_PROC_BROWSER_TEST_F(SnapHostBrowserTest, ExecuteSnapEvaluatesBundle) {
  auto* host_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUISnapsContainerURL));
  ASSERT_TRUE(host_rfh);
  auto* snap_rfh = AppendSnapHostFrame(host_rfh);
  ASSERT_TRUE(snap_rfh);

  static constexpr char kBundle[] = R"js(
    module.exports = 'npm:test-snap'
  )js";

  auto execute_result =
      content::EvalJs(host_rfh,
                      content::JsReplace(R"js(
    new Promise(resolve => {
      window.addEventListener('message', function onMsg(event) {
        if (event.data && event.data.type === 'executeSnapResult') {
          window.removeEventListener('message', onMsg);
          resolve(event.data);
        }
      });
      document.querySelector('iframe').contentWindow.postMessage({
        type: 'executeSnap',
        requestId: 1,
        payload: { snapId: 'npm:test-snap', sourceCode: $1 },
      }, 'chrome-untrusted://snap-host');
    })
  )js",
                                         kBundle),
                      content::EXECUTE_SCRIPT_NO_USER_GESTURE);
  const base::DictValue& dict = execute_result.ExtractDict();
  EXPECT_TRUE(dict.FindBool("success").value_or(false));
  ASSERT_TRUE(dict.FindString("result"));
  EXPECT_EQ("npm:test-snap", *dict.FindString("result"));
}

// Depth-3 nesting: the untrusted host inside the snaps container page inside
// the wallet page. Exercises frame-ancestors against every ancestor.
IN_PROC_BROWSER_TEST_F(SnapHostBrowserTest,
                       EmbeddableFromNestedSnapsContainer) {
  auto* wallet_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL));
  ASSERT_TRUE(wallet_rfh);
  content::RenderFrameHost* host_rfh = nullptr;
  ASSERT_TRUE(base::test::RunUntil([&] {
    host_rfh = content::ChildFrameAt(wallet_rfh, 0);
    return host_rfh &&
           host_rfh->GetLastCommittedURL() == GURL(kBraveUISnapsContainerURL);
  }));

  auto* snap_rfh = AppendSnapHostFrame(host_rfh);
  ASSERT_TRUE(snap_rfh);
  EXPECT_FALSE(snap_rfh->IsErrorDocument());
}

// An ordinary https page embedding the host must fail to load it.
IN_PROC_BROWSER_TEST_F(SnapHostBrowserTest,
                       RejectsEmbeddingFromDisallowedAncestor) {
  auto* rfh = ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("/empty.html"));
  ASSERT_TRUE(rfh);

  content::TestNavigationObserver observer(
      content::WebContents::FromRenderFrameHost(rfh));
  EXPECT_TRUE(
      content::ExecJs(rfh,
                      content::JsReplace(R"js(
      const iframe = document.createElement('iframe');
      iframe.setAttribute('sandbox', $2);
      iframe.src = $1;
      document.body.appendChild(iframe);
    )js",
                                         GURL(kUntrustedSnapURL),
                                         "allow-scripts allow-same-origin"),
                      content::EXECUTE_SCRIPT_NO_USER_GESTURE));
  observer.Wait();

  content::RenderFrameHost* subframe = content::ChildFrameAt(rfh, 0);
  ASSERT_TRUE(subframe);
  EXPECT_NE(GURL(kUntrustedSnapURL), subframe->GetLastCommittedURL());
  EXPECT_TRUE(subframe->IsErrorDocument() ||
              subframe->GetLastCommittedURL() == GURL(content::kBlockedURL));
}

// connect-src is not relaxed; outbound fetch from the host is blocked.
IN_PROC_BROWSER_TEST_F(SnapHostBrowserTest, BlocksOutboundFetch) {
  auto* host_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUISnapsContainerURL));
  ASSERT_TRUE(host_rfh);
  auto* snap_rfh = AppendSnapHostFrame(host_rfh);
  ASSERT_TRUE(snap_rfh);
  ASSERT_FALSE(snap_rfh->IsErrorDocument());

  const GURL fetch_url = https_server()->GetURL("/empty.html");
  auto result = content::EvalJs(snap_rfh,
                                content::JsReplace(R"js(
    fetch($1).then(() => 'ok').catch(e => e.name || String(e))
  )js",
                                                   fetch_url),
                                content::EXECUTE_SCRIPT_NO_USER_GESTURE);
  EXPECT_NE("ok", result.ExtractString());
}

// With the feature flag off, neither host WebUI is registered. The
// snaps-container navigation commits an error page; do not script it. A child
// about:blank#blocked commit would carry an origin derived from
// chrome://snaps-container into a process locked to
// chrome-error://chromewebdata/, which fails CanAccessOrigin.
IN_PROC_BROWSER_TEST_F(SnapHostFeatureDisabledBrowserTest,
                       HostNotRegisteredWhenFeatureDisabled) {
  snap_host::UntrustedSnapHostUIConfig config;
  EXPECT_FALSE(config.IsWebUIEnabled(browser()->GetProfile()));
  EXPECT_EQ(nullptr, content::WebUIConfigMap::GetInstance().GetConfig(
                         browser()->GetProfile(), GURL(kUntrustedSnapURL)));

  auto* snap_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kUntrustedSnapURL));
  ASSERT_TRUE(snap_rfh);
  EXPECT_TRUE(snap_rfh->IsErrorDocument());

  auto* container_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUISnapsContainerURL));
  ASSERT_TRUE(container_rfh);
  EXPECT_TRUE(container_rfh->IsErrorDocument());
}

}  // namespace brave_wallet

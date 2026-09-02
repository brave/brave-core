/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace brave_wallet {

class SnapExecutorBrowserTest : public InProcessBrowserTest {
 public:
  SnapExecutorBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveWalletSnapsFeature);
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

  content::RenderFrameHost* AppendSnapExecutorFrame(
      content::RenderFrameHost* frame) {
    return AppendSubframe(frame, GURL(kUntrustedSnapExecutorURL),
                          "allow-scripts allow-same-origin");
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

// Proves Phase 1 resource wiring and the snap_executor.html/.bundle.js fix:
// the executor loads and signals readiness to its parent.
IN_PROC_BROWSER_TEST_F(SnapExecutorBrowserTest,
                       ExecutorResourcesLoadAndSignalReady) {
  auto* wallet_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL));
  ASSERT_TRUE(wallet_rfh);

  // The assignment's value is the pending Promise itself, so this must not
  // wait on it — it only resolves once the iframe below is created.
  ASSERT_TRUE(content::ExecJs(wallet_rfh, R"js(
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

  auto* snap_rfh = AppendSnapExecutorFrame(wallet_rfh);
  ASSERT_TRUE(snap_rfh);
  EXPECT_FALSE(snap_rfh->IsErrorDocument());

  EXPECT_EQ(true, content::EvalJs(wallet_rfh, "window.executorReadyPromise"));
}

// Proves the new Function() CJS wrapper fix (Phase 0.3): a CommonJS bundle's
// module.exports.onRpcRequest is reachable, and snapRpc round-trips through
// it.
IN_PROC_BROWSER_TEST_F(SnapExecutorBrowserTest, ExecuteSnapAndRpcRoundTrip) {
  auto* wallet_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL));
  ASSERT_TRUE(wallet_rfh);
  auto* snap_rfh = AppendSnapExecutorFrame(wallet_rfh);
  ASSERT_TRUE(snap_rfh);

  static constexpr char kBundle[] = R"js(
    module.exports.onRpcRequest = async (request) => {
      const { method, params } = request
      if (method === 'echo') {
        return { echoed: params }
      }
      throw new Error('unknown method: ' + method)
    }
  )js";

  auto execute_result =
      content::EvalJs(wallet_rfh,
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
        payload: { snapId: 'npm:test-snap', sourceCode: $1, endowments: [] },
      }, 'chrome-untrusted://snap-executor');
    })
  )js",
                                         kBundle),
                      content::EXECUTE_SCRIPT_NO_USER_GESTURE);
  EXPECT_TRUE(execute_result.ExtractDict().FindBool("success").value_or(false));

  auto rpc_result = content::EvalJs(wallet_rfh, R"js(
    new Promise(resolve => {
      window.addEventListener('message', function onMsg(event) {
        if (event.data && event.data.type === 'snapRpcResult') {
          window.removeEventListener('message', onMsg);
          resolve(event.data);
        }
      });
      document.querySelector('iframe').contentWindow.postMessage({
        type: 'snapRpc',
        requestId: 2,
        payload: {
          snapId: 'npm:test-snap',
          handler: 'onRpcRequest',
          origin: 'chrome://wallet',
          request: { jsonrpc: '2.0', method: 'echo', params: { hello: 'world' } },
        },
      }, 'chrome-untrusted://snap-executor');
    })
  )js");
  const base::DictValue& rpc_dict = rpc_result.ExtractDict();
  const base::DictValue* rpc_result_field = rpc_dict.FindDict("result");
  ASSERT_TRUE(rpc_result_field);
  const base::DictValue* echoed = rpc_result_field->FindDict("echoed");
  ASSERT_TRUE(echoed);
  EXPECT_EQ("world", *echoed->FindString("hello"));
}

// Proves the executor half of Phase 4 (outbound snap.request()) with no C++
// service involved: capture the outgoing snapRequest message shape and
// reply with a canned snapRequestResult.
IN_PROC_BROWSER_TEST_F(SnapExecutorBrowserTest, SnapRequestMessageShape) {
  auto* wallet_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL));
  ASSERT_TRUE(wallet_rfh);
  auto* snap_rfh = AppendSnapExecutorFrame(wallet_rfh);
  ASSERT_TRUE(snap_rfh);

  static constexpr char kBundle[] = R"js(
    module.exports.onRpcRequest = async (request) => {
      await snap.request({
        method: 'snap_manageState',
        params: { operation: 'update', newState: { seen: request.params } },
      })
      const state = await snap.request({
        method: 'snap_manageState',
        params: { operation: 'get' },
      })
      return { fromBrowser: state }
    }
  )js";

  ASSERT_TRUE(content::EvalJs(wallet_rfh, content::JsReplace(R"js(
    new Promise(resolve => {
      window.addEventListener('message', function onMsg(event) {
        if (event.data && event.data.type === 'executeSnapResult') {
          window.removeEventListener('message', onMsg);
          resolve(event.data.success === true);
        }
      });
      document.querySelector('iframe').contentWindow.postMessage({
        type: 'executeSnap',
        requestId: 1,
        payload: { snapId: 'npm:test-snap', sourceCode: $1, endowments: [] },
      }, 'chrome-untrusted://snap-executor');
    })
  )js",
                                                             kBundle))
                  .ExtractBool());

  // Drive the round trip: install a responder that inspects the first
  // outgoing snapRequest message, then replies canned snapRequestResults for
  // both requests the bundle issues.
  auto final_result = content::EvalJs(wallet_rfh, R"js(
    new Promise(resolve => {
      const iframe = document.querySelector('iframe');
      let firstRequest = null;
      let requestCount = 0;
      window.addEventListener('message', function onMsg(event) {
        const data = event.data;
        if (!data) return;
        if (data.type === 'snapRequest') {
          requestCount++;
          if (!firstRequest) {
            firstRequest = data;
          }
          const result = data.method === 'snap_manageState'
            && data.params && data.params.operation === 'get'
            ? { seen: { n: 7 } }
            : null;
          iframe.contentWindow.postMessage({
            type: 'snapRequestResult',
            requestId: data.requestId,
            result,
            error: null,
          }, 'chrome-untrusted://snap-executor');
        } else if (data.type === 'snapRpcResult') {
          window.removeEventListener('message', onMsg);
          resolve({ firstRequest, snapRpcResult: data });
        }
      });
      iframe.contentWindow.postMessage({
        type: 'snapRpc',
        requestId: 2,
        payload: {
          snapId: 'npm:test-snap',
          handler: 'onRpcRequest',
          origin: 'chrome://wallet',
          request: { jsonrpc: '2.0', method: 'roundTrip', params: { n: 7 } },
        },
      }, 'chrome-untrusted://snap-executor');
    })
  )js");

  const base::DictValue& dict = final_result.ExtractDict();
  const base::DictValue* first_request = dict.FindDict("firstRequest");
  ASSERT_TRUE(first_request);
  EXPECT_EQ("snapRequest", *first_request->FindString("type"));
  EXPECT_TRUE(first_request->FindInt("requestId").has_value());
  EXPECT_EQ("npm:test-snap", *first_request->FindString("snapId"));
  EXPECT_EQ("snap_manageState", *first_request->FindString("method"));
  const base::DictValue* params = first_request->FindDict("params");
  ASSERT_TRUE(params);
  EXPECT_EQ("update", *params->FindString("operation"));

  const base::DictValue* snap_rpc_result = dict.FindDict("snapRpcResult");
  ASSERT_TRUE(snap_rpc_result);
  const base::DictValue* result = snap_rpc_result->FindDict("result");
  ASSERT_TRUE(result);
  EXPECT_TRUE(result->FindDict("fromBrowser"));
}

// Regression test for Phase 0.4: two concurrent snapRpc 'echo' calls must
// resolve their own promises, not each other's.
IN_PROC_BROWSER_TEST_F(SnapExecutorBrowserTest,
                       ConcurrentRpcRequestsAreCorrelated) {
  auto* wallet_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL));
  ASSERT_TRUE(wallet_rfh);
  auto* snap_rfh = AppendSnapExecutorFrame(wallet_rfh);
  ASSERT_TRUE(snap_rfh);

  static constexpr char kBundle[] = R"js(
    module.exports.onRpcRequest = async (request) => {
      return { echoed: request.params }
    }
  )js";

  ASSERT_TRUE(content::EvalJs(wallet_rfh, content::JsReplace(R"js(
    new Promise(resolve => {
      window.addEventListener('message', function onMsg(event) {
        if (event.data && event.data.type === 'executeSnapResult') {
          window.removeEventListener('message', onMsg);
          resolve(event.data.success === true);
        }
      });
      document.querySelector('iframe').contentWindow.postMessage({
        type: 'executeSnap',
        requestId: 1,
        payload: { snapId: 'npm:test-snap', sourceCode: $1, endowments: [] },
      }, 'chrome-untrusted://snap-executor');
    })
  )js",
                                                             kBundle))
                  .ExtractBool());

  auto result = content::EvalJs(wallet_rfh, R"js(
    new Promise(resolve => {
      const iframe = document.querySelector('iframe');
      const results = {};
      let count = 0;
      window.addEventListener('message', function onMsg(event) {
        if (event.data && event.data.type === 'snapRpcResult') {
          count++;
          results[event.data.requestId] = event.data.result;
          if (count === 2) {
            window.removeEventListener('message', onMsg);
            resolve(results);
          }
        }
      });
      iframe.contentWindow.postMessage({
        type: 'snapRpc',
        requestId: 10,
        payload: {
          snapId: 'npm:test-snap', handler: 'onRpcRequest',
          origin: 'chrome://wallet',
          request: { jsonrpc: '2.0', method: 'echo', params: { tag: 'first' } },
        },
      }, 'chrome-untrusted://snap-executor');
      iframe.contentWindow.postMessage({
        type: 'snapRpc',
        requestId: 20,
        payload: {
          snapId: 'npm:test-snap', handler: 'onRpcRequest',
          origin: 'chrome://wallet',
          request: { jsonrpc: '2.0', method: 'echo', params: { tag: 'second' } },
        },
      }, 'chrome-untrusted://snap-executor');
    })
  )js");

  const base::DictValue& dict = result.ExtractDict();
  const base::DictValue* first = dict.FindDict("10");
  const base::DictValue* second = dict.FindDict("20");
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  EXPECT_EQ("first", *first->FindDict("echoed")->FindString("tag"));
  EXPECT_EQ("second", *second->FindDict("echoed")->FindString("tag"));
}

// An ordinary https page embedding the executor must fail to load it — both
// because a renderer at that security principal isn't permitted to commit
// any chrome-untrusted:// URL at all (rewritten to content::kBlockedURL by
// process-level URL filtering, independent of CSP) and, for principals that
// are otherwise allowed to request chrome-untrusted:// resources,
// AddFrameAncestor's `frame-ancestors chrome://wallet/` would separately
// reject it via a CSP error-page commit. Accept either outcome and just
// assert the executor's content never loaded. Waits for the child frame's
// navigation to actually finish via TestNavigationObserver, rather than
// relying on iframe.onload, which races with (or never fires for) a blocked
// navigation.
IN_PROC_BROWSER_TEST_F(SnapExecutorBrowserTest,
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
                                         GURL(kUntrustedSnapExecutorURL),
                                         "allow-scripts allow-same-origin"),
                      content::EXECUTE_SCRIPT_NO_USER_GESTURE));
  observer.Wait();

  content::RenderFrameHost* subframe = content::ChildFrameAt(rfh, 0);
  ASSERT_TRUE(subframe);
  EXPECT_NE(GURL(kUntrustedSnapExecutorURL), subframe->GetLastCommittedURL());
  EXPECT_TRUE(subframe->IsErrorDocument() ||
              subframe->GetLastCommittedURL() == GURL(content::kBlockedURL));
}

}  // namespace brave_wallet

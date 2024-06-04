/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/thread_annotations.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/network_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "third_party/blink/public/common/features.h"

using blink::features::kBraveGlobalPrivacyControl;

enum class GPCHeaderResult {
  kOk,
  kNoHeader,
  kWrongValue,
};

class GlobalPrivacyControlNetworkDelegateBrowserTest
    : public InProcessBrowserTest {
 public:
  GlobalPrivacyControlNetworkDelegateBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &GlobalPrivacyControlNetworkDelegateBrowserTest::HandleRequest,
        base::Unretained(this)));

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(https_server_.Start());
  }

  void HandleRequest(const net::test_server::HttpRequest& request) {
    base::AutoLock auto_lock(header_result_lock_);
    if (!start_tracking_) {
      return;
    }

    auto it = request.headers.find(kSecGpcHeader);
    if (it == request.headers.end()) {
      header_result_ = GPCHeaderResult::kNoHeader;
    } else if (it->second != "1") {
      header_result_ = GPCHeaderResult::kWrongValue;
    } else {
      header_result_ = GPCHeaderResult::kOk;
    }
  }

  const net::EmbeddedTestServer& https_server() { return https_server_; }

  void StartTracking() {
    base::AutoLock auto_lock(header_result_lock_);
    ASSERT_FALSE(header_result_);
    start_tracking_ = true;
  }

  GPCHeaderResult header_result() {
    base::AutoLock auto_lock(header_result_lock_);
    EXPECT_TRUE(header_result_);
    return *header_result_;
  }

  content::EvalJsResult MessageServiceWorker(
      const content::ToRenderFrameHost& execution_target,
      std::string message) {
    constexpr char kScript[] = R"(
      messageServiceWorker($1)
    )";
    return content::EvalJs(execution_target,
                           content::JsReplace(kScript, message));
  }

 private:
  net::test_server::EmbeddedTestServer https_server_;
  mutable base::Lock header_result_lock_;
  bool start_tracking_ GUARDED_BY(header_result_lock_) = false;
  std::optional<GPCHeaderResult> header_result_ GUARDED_BY(header_result_lock_);
};

// When kGlobalPrivacyControl is enabled, the Sec-GPC flag should appear on
// request headers.
IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlNetworkDelegateBrowserTest,
                       IncludesSecGPCHeader) {
  const GURL target = https_server().GetURL("a.test", "/simple.html");
  StartTracking();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_EQ(header_result(), GPCHeaderResult::kOk);
}

// The Global Privacy Control spec also defines the
// `navigator.globalPrivacyControl` JS property, which is read-only. In Brave
// it will always return `true`.
IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlNetworkDelegateBrowserTest,
                       NavigatorGlobalPrivacyAPI) {
  const GURL target = https_server().GetURL("a.test", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));

  auto* rfh = browser()
                  ->tab_strip_model()
                  ->GetActiveWebContents()
                  ->GetPrimaryMainFrame();

  EXPECT_EQ(true, content::EvalJs(rfh, "navigator.globalPrivacyControl"));
  EXPECT_EQ(true, content::EvalJs(rfh,
                                  "(function() {"
                                  "  navigator.globalPrivacyControl = false;"
                                  "  return navigator.globalPrivacyControl;"
                                  "})()"));
}

// The Global Privacy Control spec also defines the
// `navigator.globalPrivacyControl` JS property, which is read-only. In Brave
// it will always return `true`.
IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlNetworkDelegateBrowserTest,
                       ServiceWorkerGPCAvailable) {
  const GURL target = https_server().GetURL("a.test", "/navigator/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));

  auto* rfh = browser()
                  ->tab_strip_model()
                  ->GetActiveWebContents()
                  ->GetPrimaryMainFrame();

  ASSERT_TRUE(content::ExecJs(rfh, R"(
    registerServiceWorker('./service-workers-gpc.js')
  )"));

  StartTracking();
  EXPECT_EQ(MessageServiceWorker(rfh, "fetch"), "LOADED");
  EXPECT_EQ(header_result(), GPCHeaderResult::kOk);

  EXPECT_EQ(MessageServiceWorker(rfh, "hasGpc"), true);
  EXPECT_EQ(MessageServiceWorker(rfh, "checkGpc"), true);
}

class GlobalPrivacyControlFlagDisabledTest
    : public GlobalPrivacyControlNetworkDelegateBrowserTest {
 public:
  GlobalPrivacyControlFlagDisabledTest() {
    feature_list_.InitAndDisableFeature(kBraveGlobalPrivacyControl);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// When kGlobalPrivacyControl is disabled, the Sec-GPC header shouldn't be sent.
IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlFlagDisabledTest, SecGPCHeaderNot1) {
  const GURL target = https_server().GetURL("a.test", "/simple.html");
  StartTracking();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));
  EXPECT_EQ(header_result(), GPCHeaderResult::kNoHeader);
}

// When kGlobalPrivacyControl is disabled, the `navigator.globalPrivacyControl`
// should not return true.
IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlFlagDisabledTest,
                       NavigatorGlobalPrivacyAPI) {
  const GURL target = https_server().GetURL("a.test", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), target));

  auto* rfh = browser()
                  ->tab_strip_model()
                  ->GetActiveWebContents()
                  ->GetPrimaryMainFrame();

  EXPECT_EQ(false, content::EvalJs(rfh, "navigator.globalPrivacyControl"));
}

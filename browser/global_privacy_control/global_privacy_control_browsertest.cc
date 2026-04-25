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
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
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

class GlobalPrivacyControlBrowserTest : public PlatformBrowserTest {
 public:
  GlobalPrivacyControlBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.RegisterRequestMonitor(
        base::BindRepeating(&GlobalPrivacyControlBrowserTest::HandleRequest,
                            base::Unretained(this)));

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
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

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

 private:
  net::test_server::EmbeddedTestServer https_server_;
  mutable base::Lock header_result_lock_;
  bool start_tracking_ GUARDED_BY(header_result_lock_) = false;
  std::optional<GPCHeaderResult> header_result_ GUARDED_BY(header_result_lock_);
};

// When kGlobalPrivacyControl is enabled, the Sec-GPC flag should appear on
// request headers.
IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlBrowserTest, IncludesSecGPCHeader) {
  const GURL target = https_server().GetURL("a.test", "/simple.html");
  StartTracking();
  ASSERT_TRUE(content::NavigateToURL(web_contents(), target));

  EXPECT_EQ(header_result(), GPCHeaderResult::kOk);
}

// The Global Privacy Control spec also defines the
// `navigator.globalPrivacyControl` JS property, which is read-only. In Brave
// it will always return `true`.
IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlBrowserTest,
                       NavigatorGlobalPrivacyAPI) {
  const GURL target = https_server().GetURL("a.test", "/simple.html");
  ASSERT_TRUE(content::NavigateToURL(web_contents(), target));

  auto* rfh = web_contents()->GetPrimaryMainFrame();

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
IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlBrowserTest,
                       ServiceWorkerGPCAvailable) {
  const GURL target = https_server().GetURL("a.test", "/navigator/simple.html");
  ASSERT_TRUE(content::NavigateToURL(web_contents(), target));

  auto* rfh = web_contents()->GetPrimaryMainFrame();

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
    : public GlobalPrivacyControlBrowserTest {
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
  ASSERT_TRUE(content::NavigateToURL(web_contents(), target));

  EXPECT_EQ(header_result(), GPCHeaderResult::kNoHeader);
}

// When kGlobalPrivacyControl is disabled, the `navigator.globalPrivacyControl`
// should not return true.
IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlFlagDisabledTest,
                       NavigatorGlobalPrivacyAPI) {
  const GURL target = https_server().GetURL("a.test", "/simple.html");
  ASSERT_TRUE(content::NavigateToURL(web_contents(), target));

  auto* rfh = web_contents()->GetPrimaryMainFrame();

  EXPECT_EQ(false, content::EvalJs(rfh, "navigator.globalPrivacyControl"));
}

class GlobalPrivacyControlPolicyTest : public GlobalPrivacyControlBrowserTest {
 public:
  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    EXPECT_CALL(provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(&provider_);
  }

  void SetGlobalPrivacyControlPolicy(bool enabled) {
    policy::PolicyMap policies;
    policies.Set(policy::key::kBraveGlobalPrivacyControlEnabled,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_PLATFORM, base::Value(enabled), nullptr);
    provider_.UpdateChromePolicy(policies);
  }

 private:
  policy::MockConfigurationPolicyProvider provider_;
};

IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlPolicyTest,
                       CheckNavigatorAPIAndHeaderWhenDisabledByPolicy) {
  SetGlobalPrivacyControlPolicy(/*enabled=*/false);

  const GURL target = https_server().GetURL("a.test", "/simple.html");
  StartTracking();
  ASSERT_TRUE(content::NavigateToURL(web_contents(), target));

  auto* rfh = web_contents()->GetPrimaryMainFrame();

  EXPECT_EQ(header_result(), GPCHeaderResult::kNoHeader);
  EXPECT_EQ(false, content::EvalJs(rfh, "navigator.globalPrivacyControl"));
}

IN_PROC_BROWSER_TEST_F(GlobalPrivacyControlPolicyTest,
                       CheckServiceWorkerWhenDisabledByPolicy) {
  SetGlobalPrivacyControlPolicy(/*enabled=*/false);

  const GURL target = https_server().GetURL("a.test", "/navigator/simple.html");
  ASSERT_TRUE(content::NavigateToURL(web_contents(), target));

  auto* rfh = web_contents()->GetPrimaryMainFrame();

  ASSERT_TRUE(content::ExecJs(rfh, R"(
      registerServiceWorker('./service-workers-gpc.js')
  )"));

  StartTracking();
  EXPECT_EQ(MessageServiceWorker(rfh, "fetch"), "LOADED");
  EXPECT_EQ(header_result(), GPCHeaderResult::kNoHeader);

  EXPECT_EQ(MessageServiceWorker(rfh, "hasGpc"), true);
  EXPECT_EQ(MessageServiceWorker(rfh, "checkGpc"), false);
}

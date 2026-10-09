/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/extensions/api/identity/brave_web_auth_flow.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "components/signin/public/base/signin_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/url_loader_interceptor.h"
#include "extensions/browser/background_script_executor.h"
#include "extensions/test/result_catcher.h"
#include "google_apis/google_api_keys.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/idle/idle.h"
#include "ui/base/idle/scoped_set_idle_state.h"
#include "url/origin.h"

namespace extensions {
namespace {
constexpr char kIdentityTestExtensionId[] = "igbmfgdcighdkjdgcnoaboocnjopojdh";

class IdentityExtensionApiTest : public ExtensionApiTest {
 public:
  IdentityExtensionApiTest() = default;
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    base::PathService::Get(brave::DIR_TEST_DATA, &extension_dir_);
    extension_dir_ = extension_dir_.AppendASCII("extensions/api_test");
  }
  base::FilePath extension_dir_;
};

IN_PROC_BROWSER_TEST_F(IdentityExtensionApiTest, FetchingTokenInteractiveMode) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("getAuthToken"));
  ASSERT_TRUE(extension);
  BraveWebAuthFlow::SetTokenForTesting("test_token");

  ASSERT_TRUE(extensions::BackgroundScriptExecutor::ExecuteScriptAsync(
      browser()->GetProfile(), kIdentityTestExtensionId, R"(
        chrome.identity.getAuthToken({ interactive: true }, function(token) {
          if (chrome.runtime.lastError) {
            chrome.test.fail();
            return;
          }
          if (token === "test_token") {
            chrome.test.succeed();
          } else {
            chrome.test.fail();
          }
        });
      )",
      browsertest_util::ScriptUserActivation::kDontActivate));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IdentityExtensionApiTest, FetchingTokenSilentMode) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("getAuthToken"));
  ASSERT_TRUE(extension);
  BraveWebAuthFlow::SetTokenForTesting("test_token");

  ASSERT_TRUE(extensions::BackgroundScriptExecutor::ExecuteScriptAsync(
      browser()->GetProfile(), kIdentityTestExtensionId, R"(
        chrome.identity.getAuthToken({ interactive: false }, function(token) {
          if (chrome.runtime.lastError) {
            chrome.test.fail();
            return;
          }
          if (token === "test_token") {
            chrome.test.succeed();
          } else {
            chrome.test.fail();
          }
        });
      )",
      browsertest_util::ScriptUserActivation::kDontActivate));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

// Exercises the real `WebAuthFlow` (no token override) to verify that the
// calling extension's origin reaches the provider navigation as its initiator,
// and only when `kExtensionWebAuthFlowInitiatorOrigin` is enabled.
class IdentityWebAuthFlowInitiatorTest
    : public IdentityExtensionApiTest,
      public testing::WithParamInterface<bool> {
 public:
  IdentityWebAuthFlowInitiatorTest() {
    scoped_feature_list_.InitWithFeatureState(
        switches::kExtensionWebAuthFlowInitiatorOrigin, GetParam());
  }

  void SetUpOnMainThread() override {
    IdentityExtensionApiTest::SetUpOnMainThread();
    // The test extension's background is a service worker, which has no
    // mechanism for simulating a user gesture (unlike a background page), so
    // the interactive flow's gesture-or-idle-state gate is satisfied by
    // pinning the idle state instead, matching upstream's own identity
    // browser tests.
    idle_state_ = std::make_unique<ui::ScopedSetIdleState>(
        ui::IdleState::IDLE_STATE_ACTIVE);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<ui::ScopedSetIdleState> idle_state_;
};

IN_PROC_BROWSER_TEST_P(IdentityWebAuthFlowInitiatorTest,
                       ProviderNavigationInitiator) {
  if (google_apis::IsGoogleChromeAPIKeyUsed()) {
    GTEST_SKIP() << "WebAuthFlow is only used without Google Chrome API keys.";
  }

  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("getAuthToken"));
  ASSERT_TRUE(extension);

  std::optional<std::optional<url::Origin>> initiator;
  base::RunLoop run_loop;
  content::URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](content::URLLoaderInterceptor::RequestParams* params) {
        const GURL& url = params->url_request.url;
        if (url.host() != "accounts.google.com" ||
            url.path() != "/o/oauth2/v2/auth") {
          return false;
        }
        initiator = params->url_request.request_initiator;
        content::URLLoaderInterceptor::WriteResponse(
            "HTTP/1.1 200 OK\nContent-Type: text/html\n\n", "<html></html>",
            params->client.get());
        run_loop.Quit();
        return true;
      }));

  ASSERT_TRUE(extensions::BackgroundScriptExecutor::ExecuteScriptAsync(
      browser()->GetProfile(), kIdentityTestExtensionId,
      "chrome.identity.getAuthToken({ interactive: true }, () => {});",
      browsertest_util::ScriptUserActivation::kDontActivate));
  run_loop.Run();

  ASSERT_TRUE(initiator.has_value());
  if (GetParam()) {
    EXPECT_EQ(*initiator, extension->origin());
  } else {
    EXPECT_NE(*initiator, extension->origin());
  }
}

INSTANTIATE_TEST_SUITE_P(All,
                         IdentityWebAuthFlowInitiatorTest,
                         testing::Bool());

}  // namespace
}  // namespace extensions

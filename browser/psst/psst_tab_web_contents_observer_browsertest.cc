// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_tab_web_contents_observer.h"

#include <memory>

#include "base/containers/contains.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/tabs/public/tab_features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/psst/browser/content/psst_scripts_result_handler.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/psst_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

class MockScriptHandler : public PsstScriptsHandler {
 public:
  MockScriptHandler() = default;
  ~MockScriptHandler() override = default;

  MOCK_METHOD(void, Start, (), (override));
  MOCK_METHOD(PsstDialogDelegate*, GetPsstDialogDelegate, (), (override));
  MOCK_METHOD(mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>&,
              GetRemote,
              (content::RenderFrameHost * rfh),
              (override));
  MOCK_METHOD(void,
              InsertUserScript,
              (const std::optional<MatchedRule>& rule),
              (override));
  MOCK_METHOD(void,
              OnUserScriptResult,
              (const MatchedRule& rule, base::Value script_result),
              (override));
  MOCK_METHOD(void,
              InsertScriptInPage,
              (const std::string& script,
               std::optional<base::Value> value,
               InsertScriptInPageCallback cb),
              (override));
};

class PsstTabWebContentsObserverBrowserTest : public PlatformBrowserTest {
 public:
  PsstTabWebContentsObserverBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(psst::features::kBravePsst);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);

    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());

    auto* registry = PsstRuleRegistryAccessor::GetInstance()->Registry();
    if (registry) {
      registry->LoadRules(test_data_dir.AppendASCII("psst-component-data"));
    }
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }

  net::EmbeddedTestServer& GetEmbeddedTestServer() { return https_server_; }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  MockScriptHandler* MockPsstDialogTabHelperDelegate() {
    auto* psst_tab_helper = browser()
                                ->GetActiveTabInterface()
                                ->GetTabFeatures()
                                ->psst_web_contents_observer();
    auto script_handler = std::make_unique<MockScriptHandler>();
    MockScriptHandler* result = script_handler.get();
    psst_tab_helper->SetScriptHandlerForTesting(std::move(script_handler));
    return result;
  }

 private:
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// TESTS
IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       DontStartScriptHandlerForSameDocument) {
  psst::SetEnablePsstFlag(GetPrefs(), true);
  EXPECT_EQ(psst::GetEnablePsstFlag(GetPrefs()), true);
  const GURL url = GetEmbeddedTestServer().GetURL("a.com", "/simple.html");

  auto* mocked_script_handler = MockPsstDialogTabHelperDelegate();
  EXPECT_CALL(*mocked_script_handler, Start()).Times(1).WillOnce([&]() {
    EXPECT_TRUE(base::Contains(web_contents()->GetLastCommittedURL().spec(),
                               "simple.html"));
  });
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));

  const GURL same_doc_url =
      GetEmbeddedTestServer().GetURL("a.com", "/simple.html#1");
  EXPECT_CALL(*mocked_script_handler, Start()).Times(0);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), same_doc_url));
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       DontStartScriptHandlerIfPsstDisabled) {
  psst::SetEnablePsstFlag(GetPrefs(), false);
  EXPECT_EQ(psst::GetEnablePsstFlag(GetPrefs()), false);
  const GURL url = GetEmbeddedTestServer().GetURL("a.com", "/simple.html");

  auto* mocked_script_handler = MockPsstDialogTabHelperDelegate();
  EXPECT_CALL(*mocked_script_handler, Start()).Times(0);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerOnlyInPrimaryMainFrame) {
  psst::SetEnablePsstFlag(GetPrefs(), true);
  EXPECT_EQ(psst::GetEnablePsstFlag(GetPrefs()), true);
  const GURL url = GetEmbeddedTestServer().GetURL("a.com", "/iframe_load.html");
  const GURL ifrave_url =
      GetEmbeddedTestServer().GetURL("a.com", "/simple.html");

  auto* mocked_script_handler = MockPsstDialogTabHelperDelegate();
  EXPECT_CALL(*mocked_script_handler, Start()).Times(1).WillOnce([&]() {
    EXPECT_TRUE(base::Contains(web_contents()->GetLastCommittedURL().spec(),
                               "iframe_load.html"));
  });

  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_CALL(*mocked_script_handler, Start()).Times(0);
  ASSERT_TRUE(content::NavigateIframeToURL(web_contents(), "test", ifrave_url));
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerOnlyIfCommittedNavigation) {
  const GURL url("https://unknows.address.com/simple.html");
  auto* mocked_script_handler = MockPsstDialogTabHelperDelegate();
  EXPECT_CALL(*mocked_script_handler, Start()).Times(0);
  ASSERT_FALSE(content::NavigateToURL(web_contents(), url));
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerBothScriptsExecuted) {
  psst::SetEnablePsstFlag(GetPrefs(), true);
  EXPECT_EQ(psst::GetEnablePsstFlag(GetPrefs()), true);
  const GURL url = GetEmbeddedTestServer().GetURL("a.com", "/simple.html");

  std::u16string expected_title(u"user-policy");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerJustUserScriptExecuted) {
  psst::SetEnablePsstFlag(GetPrefs(), true);
  EXPECT_EQ(psst::GetEnablePsstFlag(GetPrefs()), true);
  const GURL url = GetEmbeddedTestServer().GetURL("b.com", "/simple.html");

  std::u16string expected_title(u"user-");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerNoMatchedRule) {
  psst::SetEnablePsstFlag(GetPrefs(), true);
  EXPECT_EQ(psst::GetEnablePsstFlag(GetPrefs()), true);
  const GURL url = GetEmbeddedTestServer().GetURL("c.com", "/simple.html");

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}
}  // namespace psst

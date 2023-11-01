// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/psst/browser/core/psst_rule_service.h"
#include "brave/components/psst/common/features.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#endif

class PsstTabHelperBrowserTest : public PlatformBrowserTest {
 public:
  PsstTabHelperBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(psst::features::kBravePsst);

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    psst::PsstRuleService::GetInstance()->SetComponentPathForTest(
        test_data_dir.AppendASCII("psst-component-data"));
    https_server_.ServeFilesFromDirectory(test_data_dir);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());
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

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 protected:
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// TESTS

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, RuleMatchTestScriptTrue) {
  const GURL url = https_server_.GetURL("a.com", "/simple.html");

  const char rules[] =
      R"(
      [
        {
            "include": [
                "https://a.com/*"
            ],
            "exclude": [
            ],
            "version": 1,
            "test_script": "a/test.js",
            "policy_script": "a/policy.js"
        }
      ]
      )";
  psst::PsstRuleService::GetInstance()->OnFileDataReady(rules);

  std::u16string expected_title(u"testpolicy");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, RuleMatchTestScriptFalse) {
  const GURL url = https_server_.GetURL("b.com", "/simple.html");

  const char rules[] =
      R"(
      [
        {
            "include": [
                "https://b.com/*"
            ],
            "exclude": [
            ],
            "version": 1,
            "test_script": "b/test.js",
            "policy_script": "b/policy.js"
        }
      ]
      )";
  psst::PsstRuleService::GetInstance()->OnFileDataReady(rules);

  std::u16string expected_title(u"test");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, NoMatch) {
  const GURL url = https_server_.GetURL("a.com", "/simple.html");

  const char rules[] =
      R"(
      [
        {
            "include": [
                "https://c.com/*"
            ],
            "exclude": [
            ],
            "version": 1,
            "test_script": "a/test.js",
            "policy_script": "a/policy.js"
        }
      ]
      )";
  psst::PsstRuleService::GetInstance()->OnFileDataReady(rules);

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

class PsstTabHelperBrowserTestDisabled : public PsstTabHelperBrowserTest {
 public:
  PsstTabHelperBrowserTestDisabled() {
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(psst::features::kBravePsst);
  }
};

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTestDisabled, DoesNotInjectScript) {
  const GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_FALSE(psst::PsstRuleService::GetInstance());

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

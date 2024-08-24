// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace psst {

class PsstTabHelperBrowserTest : public PlatformBrowserTest {
 public:
  PsstTabHelperBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(psst::features::kBravePsst);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    // Also called in Disabled test.
    if (psst::PsstRuleRegistry::GetInstance()) {
      psst::PsstRuleRegistry::GetInstance()->SetComponentPath(
          test_data_dir.AppendASCII("psst-component-data"));
    }
    https_server_.ServeFilesFromDirectory(test_data_dir);

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
    return chrome_test_utils::GetActiveWebContents(this);
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
  psst::PsstRuleRegistry::GetInstance()->OnLoadRules(rules);

  std::u16string expected_title(u"testpolicy");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
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
  psst::PsstRuleRegistry::GetInstance()->OnLoadRules(rules);

  std::u16string expected_title(u"test");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
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
  psst::PsstRuleRegistry::GetInstance()->OnLoadRules(rules);

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
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
  ASSERT_FALSE(psst::PsstRuleRegistry::GetInstance());

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

}  // namespace psst

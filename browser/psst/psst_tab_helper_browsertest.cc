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
#include "chrome/test/base/ui_test_utils.h"
#include "components/javascript_dialogs/app_modal_dialog_controller.h"
#include "components/javascript_dialogs/app_modal_dialog_view.h"
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

  void LoadRulesForTest(const std::string& contents) {
    psst::PsstRuleRegistry::GetInstance()->OnLoadRules(contents);
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
            "name": "a",
            "version": 1,
            "user_script": "user.js",
            "test_script": "test.js",
            "policy_script": "policy.js"
        }
      ]
      )";
  LoadRulesForTest(rules);

  // The title is built up by the 3 scripts.
  std::u16string expected_title(u"user-test-policy");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  javascript_dialogs::AppModalDialogController* dialog =
      ui_test_utils::WaitForAppModalDialog();
  dialog->view()->AcceptAppModalDialog();
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
  // TODO(ssahib): check for pref state update.
}

// IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, RuleMatchTestScriptFalse) {
//   const GURL url = https_server_.GetURL("b.com", "/simple.html");

//   const char rules[] =
//       R"(
//       [
//         {
//             "include": [
//                 "https://b.com/*"
//             ],
//             "exclude": [
//             ],
//             "name": "b",
//             "version": 1,
//             "user_script": "user.js",
//             "test_script": "test.js",
//             "policy_script": "policy.js"
//         }
//       ]
//       )";
//   LoadRulesForTest(rules);

//   // Wait and accept the dialog.
//   views::NamedWidgetShownWaiter psst_consent_dialog_waiter(
//       views::test::AnyWidgetTestPasskey{}, "PsstConsentDialog");
//   auto* psst_consent_dialog_widget =
//   psst_consent_dialog_waiter.WaitIfNeededAndGet();
//   EXPECT_NE(psst_consent_dialog_widget, nullptr);

//   // The policy script does not run but user and test do.
//   std::u16string expected_title(u"user-test-");
//   content::TitleWatcher watcher(web_contents(), expected_title);
//   ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
//   EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
// }

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
            "name" : "c",
            "version": 1,
            "user_script": "user.js",
            "test_script": "test.js",
            "policy_script": "policy.js"
        }
      ]
      )";
  LoadRulesForTest(rules);

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, UserNotFound) {
  const GURL url = https_server_.GetURL("d.com", "/simple.html");

  const char rules[] =
      R"(
      [
        {
            "include": [
                "https://d.com/*"
            ],
            "exclude": [
            ],
            "name": "d",
            "version": 1,
            "user_script": "user.js",
            "test_script": "test.js",
            "policy_script": "policy.js"
        }
      ]
      )";
  LoadRulesForTest(rules);

  // The policy script does not run but user and test do.
  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, NoInsertIfNoName) {
  const GURL url = https_server_.GetURL("c.com", "/simple.html");

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
            "test_script": "test.js",
            "policy_script": "policy.js"
        }
      ]
      )";
  LoadRulesForTest(rules);

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

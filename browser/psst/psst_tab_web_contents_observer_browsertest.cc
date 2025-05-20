// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>

#include "base/base64.h"
#include "base/base64url.h"
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
#include "brave/components/psst/common/pref_names.h"
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

namespace {
constexpr char kResetPageTitleScript[] =
    R"(document.title='NO_TITLE'; document.title)";
constexpr char kGetCurrentUrlScript[] = R"(window.location.href)";

GURL url(const GURL& destination_url, const GURL& navigation_url) {
  std::string encoded_destination;
  base::Base64UrlEncode(destination_url.spec(),
                        base::Base64UrlEncodePolicy::OMIT_PADDING,
                        &encoded_destination);
  const std::string query =
      base::StringPrintf("url=%s", encoded_destination.c_str());
  GURL::Replacements replacement;
  replacement.SetQueryStr(query);
  return navigation_url.ReplaceComponents(replacement);
}
}  // namespace

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

    PsstRuleRegistry::GetInstance()->LoadRules(
        test_data_dir.AppendASCII("psst-component-data"));
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

 private:
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// TESTS
IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       DontStartScriptHandlerForSameDocument) {
  psst::SetPsstEnabledState(GetPrefs(), true);
  EXPECT_EQ(psst::IsPsstEnabled(GetPrefs()), true);
  const GURL url = GetEmbeddedTestServer().GetURL("a.com", "/simple.html");

  {
    std::u16string expected_title(u"a_user-a_policy");
    content::TitleWatcher watcher(web_contents(), expected_title);
    ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
    EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
    EXPECT_EQ(
        url.spec(),
        content::EvalJs(web_contents(), kGetCurrentUrlScript).ExtractString());
  }

  {
    std::u16string expected_title(u"NO_TITLE");
    content::TitleWatcher watcher(web_contents(), expected_title);
    EXPECT_EQ(
        "NO_TITLE",
        content::EvalJs(web_contents(), kResetPageTitleScript).ExtractString());
    EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
  }
  {
    std::u16string expected_title(u"OK");
    content::TitleWatcher watcher(web_contents(), expected_title);
    ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
    EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
    EXPECT_EQ(
        url.spec(),
        content::EvalJs(web_contents(), kGetCurrentUrlScript).ExtractString());
  }
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       DontStartScriptHandlerIfPsstDisabled) {
  psst::SetPsstEnabledState(GetPrefs(), false);
  EXPECT_EQ(psst::IsPsstEnabled(GetPrefs()), false);
  const GURL url = GetEmbeddedTestServer().GetURL("a.com", "/simple.html");

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
  EXPECT_EQ(
      url.spec(),
      content::EvalJs(web_contents(), kGetCurrentUrlScript).ExtractString());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerOnlyInPrimaryMainFrame) {
  psst::SetPsstEnabledState(GetPrefs(), true);
  EXPECT_EQ(psst::IsPsstEnabled(GetPrefs()), true);
  const GURL ifrave_url =
      GetEmbeddedTestServer().GetURL("a.com", "/simple.html");
  const GURL navigate_url = url(
      ifrave_url, GetEmbeddedTestServer().GetURL("a.com", "/iframe_load.html"));
  std::u16string expected_title(u"iframe test");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), navigate_url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerBothScriptsExecuted) {
  psst::SetPsstEnabledState(GetPrefs(), true);
  EXPECT_EQ(psst::IsPsstEnabled(GetPrefs()), true);
  const GURL url = GetEmbeddedTestServer().GetURL("a.com", "/simple.html");

  std::u16string expected_title(u"a_user-a_policy");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerJustUserScriptExecuted) {
  psst::SetPsstEnabledState(GetPrefs(), true);
  EXPECT_EQ(psst::IsPsstEnabled(GetPrefs()), true);
  const GURL url = GetEmbeddedTestServer().GetURL("b.com", "/simple.html");

  std::u16string expected_title(u"b_user-");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerNoMatchedRule) {
  psst::SetPsstEnabledState(GetPrefs(), true);
  EXPECT_EQ(psst::IsPsstEnabled(GetPrefs()), true);
  const GURL url = GetEmbeddedTestServer().GetURL("c.com", "/simple.html");

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}
}  // namespace psst

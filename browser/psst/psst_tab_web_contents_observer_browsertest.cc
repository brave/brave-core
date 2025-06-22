// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include "base/base64url.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/tabs/public/tab_features.h"
#include "brave/components/psst/browser/core/psst_component_installer.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {
constexpr char kResetPageTitleScript[] =
    R"(document.title='NO_TITLE'; document.title)";
constexpr char kGetCurrentUrlScript[] = R"(window.location.href)";

GURL GetTestUrl(const GURL& destination_url, const GURL& navigation_url) {
  std::string encoded_destination;
  base::Base64UrlEncode(destination_url.spec(),
                        base::Base64UrlEncodePolicy::OMIT_PADDING,
                        &encoded_destination);
  const std::string query =
      base::JoinString({"url", encoded_destination.c_str()}, "=");
  GURL::Replacements replacement;
  replacement.SetQueryStr(query);
  return navigation_url.ReplaceComponents(replacement);
}

}  // namespace

class MockComponentUpdateService
    : public component_updater::MockComponentUpdateService {
 public:
  MockComponentUpdateService() = default;
  ~MockComponentUpdateService() override = default;
};
class PsstTabWebContentsObserverBrowserTest : public PlatformBrowserTest {
 public:
  using LoadRulesTestCallback = base::MockCallback<base::OnceCallback<
      void(const std::string& data, const std::vector<PsstRule>& rules)>>;

  PsstTabWebContentsObserverBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(psst::features::kEnablePsst);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);

    base::PathService::Override(
        component_updater::DIR_COMPONENT_PREINSTALLED,
        test_data_dir.AppendASCII("brave/components/test/data/psst"));

    https_server_.ServeFilesFromDirectory(
        test_data_dir.AppendASCII("brave/test/data"));
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());

    SetUpPsstComponent();
  }

  virtual void SetUpPsstComponent() {
    LoadRulesTestCallback mock_callback;
    base::RunLoop run_loop;
    PsstRuleRegistry::GetInstance()->SetOnLoadCallbackForTesting(
        mock_callback.Get());
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillOnce(
            [&](const std::string& data, const std::vector<PsstRule>& rules) {
              EXPECT_FALSE(rules.empty());
              EXPECT_FALSE(data.empty());
              run_loop.Quit();
            });

    RegisterPsstComponent(&component_update_service_);
    run_loop.Run();
    EXPECT_CALL(component_update_service_, RegisterComponent(testing::_))
        .Times(1)
        .WillOnce(testing::Return(true));
  }

  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }

  net::EmbeddedTestServer& GetEmbeddedTestServer() { return https_server_; }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

 protected:
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;
  MockComponentUpdateService component_update_service_;
};

// TESTS
IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       DontStartScriptHandlerForSameDocument) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  EXPECT_EQ(GetPrefs()->GetBoolean(prefs::kPsstEnabled), true);
  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/simple.html");

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
                       DontStartScriptsIfPsstDisabled) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, false);
  EXPECT_EQ(GetPrefs()->GetBoolean(prefs::kPsstEnabled), false);
  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/simple.html");

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
  EXPECT_EQ(
      url.spec(),
      content::EvalJs(web_contents(), kGetCurrentUrlScript).ExtractString());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptsOnlyInPrimaryMainFrame) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  EXPECT_EQ(GetPrefs()->GetBoolean(prefs::kPsstEnabled), true);
  const GURL iframe_url =
      GetEmbeddedTestServer().GetURL("a.test", "/simple.html");
  const GURL navigate_url =
      GetTestUrl(iframe_url,
                 GetEmbeddedTestServer().GetURL("a.test", "/iframe_load.html"));
  std::u16string expected_title(u"iframe test");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), navigate_url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTest,
                       StartScriptHandlerBothScriptsExecuted) {
  GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  EXPECT_EQ(GetPrefs()->GetBoolean(prefs::kPsstEnabled), true);
  const GURL url = GetEmbeddedTestServer().GetURL("a.test", "/simple.html");

  std::u16string expected_title(u"a_user-a_policy");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

class PsstTabWebContentsObserverBrowserTestDisabled
    : public PsstTabWebContentsObserverBrowserTest {
 public:
  PsstTabWebContentsObserverBrowserTestDisabled() {
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(psst::features::kEnablePsst);
  }

  void SetUpPsstComponent() override {
    LoadRulesTestCallback mock_callback;
    PsstRuleRegistry::GetInstance()->SetOnLoadCallbackForTesting(
        mock_callback.Get());
    EXPECT_CALL(mock_callback, Run).Times(0);

    RegisterPsstComponent(&component_update_service_);
    EXPECT_CALL(component_update_service_, RegisterComponent(testing::_))
        .Times(0);
  }
};

IN_PROC_BROWSER_TEST_F(PsstTabWebContentsObserverBrowserTestDisabled,
                       PsstTabWebContentsObserverNotCreated) {
  auto* active_tab = browser()->GetActiveTabInterface();
  ASSERT_TRUE(active_tab);
  auto* registry = active_tab->GetTabFeatures();
  ASSERT_TRUE(registry);

  // Verify that the PSST tab web contents observer is not created
  EXPECT_FALSE(registry->psst_web_contents_observer());
}

}  // namespace psst

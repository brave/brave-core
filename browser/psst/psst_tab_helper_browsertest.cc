// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cstddef>
#include <optional>
#include <string>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/views/psst/psst_consent_dialog.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/psst_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/javascript_dialogs/app_modal_dialog_controller.h"
#include "components/javascript_dialogs/app_modal_dialog_view.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "gtest/gtest.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "brave/browser/ui/views/psst/psst_consent_dialog_tracker.h"
#include "ui/views/widget/widget_delegate.h"
#include "base/strings/string_util.h"
#include "brave/components/psst/browser/core/rule_data_reader.h"
namespace psst {
namespace {
constexpr char kConsentStatus[] = "consent_status";
constexpr char kScriptVersion[] = "script_version";

std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}
}  // namespace
class MockRuleDataReader : public RuleDataReader {
 public:
  MockRuleDataReader(const base::FilePath& component_path);
  ~MockRuleDataReader() override = default;

  MOCK_METHOD(std::optional<std::string>,
              ReadUserScript,
              (const PsstRule& rule),
              (override, const));
  MOCK_METHOD(std::optional<std::string>,
              ReadTestScript,
              (const PsstRule& rule),
              (override, const));
  MOCK_METHOD(std::optional<std::string>,
              ReadPolicyScript,
              (const PsstRule& rule),
              (override, const));
};
MockRuleDataReader::MockRuleDataReader(const base::FilePath& component_path)
    : RuleDataReader(component_path) {}

class MockPsstRuleRegistryImpl : public PsstRuleRegistryImpl {
 public:
  MOCK_METHOD(void, OnLoadRules, (const std::string& data), (override));
};

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
    test_data_dir_base_ = test_data_dir.AppendASCII("psst-component-data");    
    https_server_.ServeFilesFromDirectory(test_data_dir);

    auto rule_data_reader_mock = std::make_unique<MockRuleDataReader>(
        base::FilePath(test_data_dir_base_));
    rule_data_reader_mock_ = rule_data_reader_mock.get();

    auto psst_rul_registry = std::make_unique<MockPsstRuleRegistryImpl>();
    psst_rul_registry_ = psst_rul_registry.get();
    PsstRuleRegistryAccessor::GetInstance()->SetRegistryForTesting(
        std::move(psst_rul_registry));

    psst_rul_registry_->SetRuleDataReaderForTest(
        std::move(rule_data_reader_mock));

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());

    PsstConsentDialogTracker::CreateForWebContents(web_contents());
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
    psst::PsstRuleRegistryAccessor::GetInstance()->Registry()->OnLoadRules(contents);
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void WaitUntil(base::RepeatingCallback<bool()> condition, int wait_ms = 100) {
    // if (condition.Run())
    //   return;

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition] {
                      if (condition.Run())
                        run_loop_->Quit();
                    }));
    Run();
  }

void MockOnLoadRules(const std::string& user_script, const std::string rule_file_name, std::optional<const std::string> policy_script = std::nullopt) {
  base::RunLoop loop;
      EXPECT_CALL(*rule_data_reader_mock_, ReadUserScript)
        .Times(1)
        .WillOnce(testing::Invoke([&](const PsstRule& rule) {
          return user_script;
        }));

    EXPECT_CALL(*rule_data_reader_mock_, ReadPolicyScript)
        .Times(1)
        .WillOnce(testing::Invoke([policy_script](const PsstRule& rule) {
          LOG(INFO) << "[PSST] ReadPolicyScript rule:" << rule.Name() << " policy_script:" << (policy_script.has_value() ? policy_script.value() : "console.log('policy_script');");
          return policy_script.has_value() ? policy_script.value() : "console.log('policy_script');";
        }));

     EXPECT_CALL(*psst_rul_registry_, OnLoadRules(testing::_))
        .WillOnce(testing::Invoke([&](const std::string& data) {
          EXPECT_EQ(data, ReadFile(test_data_dir_base_.Append(rule_file_name)));
          (*psst_rul_registry_).PsstRuleRegistryImpl::OnLoadRules(data);
          loop.Quit();
        }));
    PsstRuleRegistryAccessor::GetInstance()->Registry()->LoadRules(
        test_data_dir_base_);
    loop.Run();
}

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop()->Run();
  }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }

  void CloseDialog(PsstConsentDialogTracker* tracker) {
    auto* dlg = static_cast<PsstConsentDialog*>(
      tracker->active_dialog()->widget_delegate());
    if(dlg) {
      LOG(INFO) << "[PSST] CloseDialog";
      dlg->OnConsentClicked();
      dlg->AcceptDialog();
    }
  }

 protected:
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;
  base::FilePath test_data_dir_base_;
  raw_ptr<MockRuleDataReader> rule_data_reader_mock_;
  raw_ptr<MockPsstRuleRegistryImpl> psst_rul_registry_{nullptr};

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<base::RunLoop> run_loop_;
};

// TESTS

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, GetUserIdAndShowDialog) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const std::string rule_name{"a"};
  const GURL url = https_server_.GetURL(
      base::ReplaceStringPlaceholders("$1.com", {rule_name}, nullptr),
      "/simple.html");

  const std::string user_id{"user12345"};
  const std::string user_script{
      base::ReplaceStringPlaceholders(R"((() => {return {
        'user': "$1",
        "requests": [
            { 
                url:'https://$2.com/settings/ads_preferences',
                description: 'Ads Preferences'
            },
        ]
    }})())",
                                      {user_id, rule_name}, nullptr)};

  const std::string policy_script{R"((() => {return {
      "result": true,
      "psst": {},
  }})())"};
  MockOnLoadRules(user_script, "psst.json", policy_script);

  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));

  auto* dialog_tracker =
      PsstConsentDialogTracker::FromWebContents(web_contents());

  // Wait till text recognition dialog is launched.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !!dialog_tracker->active_dialog(); }));

  LOG(INFO) << "[PSST] PsstTabHelperBrowserTest.RuleMatchTestScriptTrue #200";
  ASSERT_TRUE(dialog_tracker->active_dialog());
  LOG(INFO) << "[PSST] PsstTabHelperBrowserTest.RuleMatchTestScriptTrue #300";
  CloseDialog(dialog_tracker);

  WaitUntil(base::BindLambdaForTesting([&]() {
    return !dialog_tracker || !dialog_tracker->active_dialog() ||
           dialog_tracker->active_dialog()->IsClosed();
  }));
  LOG(INFO) << "[PSST] PsstTabHelperBrowserTest.RuleMatchTestScriptTrue #400";

  const auto& psst_settings = GetPrefs()->GetDict(prefs::kPsstSettingsPref);
  LOG(INFO) << "[PSST] psst_settings:" << psst_settings.DebugString();
  auto* rule_dict = psst_settings.FindDict(rule_name);
  ASSERT_TRUE(rule_dict);

  auto* user_id_dict = rule_dict->FindDict(user_id);
  ASSERT_TRUE(user_id_dict);

  auto consent_status_val = user_id_dict->FindInt(kConsentStatus);
  auto script_version_val = user_id_dict->FindInt(kScriptVersion);
  ASSERT_TRUE(consent_status_val);
  ASSERT_TRUE(script_version_val);
  EXPECT_EQ(consent_status_val.value(), 1);
  EXPECT_EQ(script_version_val.value(), 1);
}

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, UserScriptReturnsUndefinedNoDialog) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  const std::string rule_name{"a"};
  const GURL url = https_server_.GetURL(
      base::ReplaceStringPlaceholders("$1.com", {rule_name}, nullptr),
      "/simple.html");

  const std::string user_script{
      base::ReplaceStringPlaceholders(R"((() => {return null})())",
                                      { rule_name }, nullptr)};

  MockOnLoadRules(user_script, "psst.json");

  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));

  auto* dialog_tracker =
      PsstConsentDialogTracker::FromWebContents(web_contents());

  // Wait till text recognition dialog is launched.
  WaitUntil(base::BindLambdaForTesting(
                [&]() { return !dialog_tracker->active_dialog(); }),
            3000);

  LOG(INFO) << "[PSST] PsstTabHelperBrowserTest.RuleMatchTestScriptTrue #200";
  ASSERT_FALSE(dialog_tracker->active_dialog());
}

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, UserScriptNoUserReturnedNoDialog) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  const std::string rule_name{"a"};
  const GURL url = https_server_.GetURL(
      base::ReplaceStringPlaceholders("$1.com", {rule_name}, nullptr),
      "/simple.html");

  const std::string user_script{
      base::ReplaceStringPlaceholders(R"((() => {return {
        'user': undefined,
        "requests": [
            { 
                url:'https://$1.com/settings/ads_preferences',
                description: 'Ads Preferences'
            },
        ]
    }})())",
                                      { rule_name }, nullptr)};

  MockOnLoadRules(user_script, "psst.json");

  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));

  auto* dialog_tracker =
      PsstConsentDialogTracker::FromWebContents(web_contents());

  // Wait till text recognition dialog is launched.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !dialog_tracker->active_dialog(); }), 3000);

  LOG(INFO) << "[PSST] PsstTabHelperBrowserTest.RuleMatchTestScriptTrue #200";
  ASSERT_FALSE(dialog_tracker->active_dialog());
}

IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, NavigateToAnotherUrlNoPsstRuleNoDialog) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  const std::string rule_name{"a"};
  const GURL url = https_server_.GetURL("url_with_no_rule.com", "/simple.html");
LOG(INFO) << "[PSST] NavigateToAnotherUrlNoPsstNoDialog url:" << url;
  const std::string user_script{
      base::ReplaceStringPlaceholders(R"((() => {return {
        'user': undefined,
        "requests": [
            { 
                url:'https://$1.com/settings/ads_preferences',
                description: 'Ads Preferences'
            },
        ]
    }})())",
                                      { rule_name }, nullptr)};

  MockOnLoadRules(user_script, "psst.json");

  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));

  auto* dialog_tracker =
      PsstConsentDialogTracker::FromWebContents(web_contents());

  // Wait till text recognition dialog is launched.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !dialog_tracker->active_dialog(); }), 3000);

  LOG(INFO) << "[PSST] PsstTabHelperBrowserTest.RuleMatchTestScriptTrue #200";
  ASSERT_FALSE(dialog_tracker->active_dialog());
}


IN_PROC_BROWSER_TEST_F(PsstTabHelperBrowserTest, CallTestScriptAsDialogAccepted) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  const std::string user_id{"user12345"};
  ASSERT_TRUE(SetPsstSettings(user_id, "a", PsstSettings{psst::kAllow, 1},
  GetPrefs()));
  const auto& psst_settings = GetPrefs()->GetDict(prefs::kPsstSettingsPref);
  LOG(INFO) << "[PSST] psst_settings:" << psst_settings.DebugString();

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
  ASSERT_FALSE(psst::PsstRuleRegistryAccessor::GetInstance()->Registry());

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

}  // namespace psst

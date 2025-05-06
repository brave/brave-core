/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/content/psst_scripts_result_handler.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/to_string.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/browser/core/rule_data_reader.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/psst_constants.h"
#include "brave/components/psst/common/psst_prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;
namespace {
std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}

constexpr char kUserId[] = "user12345";
constexpr char kShareExperienceLink[] = "http://test.link/post=$1";
constexpr char kName[] = "a";
constexpr char kCorrectUserScriptResult[] = R"({
    "user": "$1",
    "share_experience_link": "$2",
    "name": "$3",
    "tasks": [
        {
            "url": "https://x.com/settings/ads_preferences",
            "description": "Disable personalized ads"
        }
    ]
})";

constexpr char kNoUserPropUserScriptResult[] = R"({
  "share_experience_link": "$1",
  "name": "$2",
  "tasks": [
      {
          "url": "https://x.com/settings/ads_preferences",
          "description": "Disable personalized ads"
      }
  ]
})";

constexpr char kNoTasksUserScriptResult[] = R"({
  "user": "$1",
  "share_experience_link": "$2",
  "name": "$3",
})";

constexpr char kEmptyTasksUserScriptResult[] = R"({
  "share_experience_link": "$1",
  "name": "$2",
  "tasks": []
})";

constexpr char kNoNameUserScriptResult[] = R"({
  "user": "$1",
  "share_experience_link": "$2",
  "tasks": [
      {
          "url": "https://x.com/settings/ads_preferences",
          "description": "Disable personalized ads"
      }
  ]
})";

constexpr char kPolicyScriptScriptResult[] = R"({
   "psst": {
      "applied": [ {
         "description": "It should be failed",
         "url": "https://x.com/settings/ads_preferences12345"
      }, {
         "description": "Disable attaching location information to posts",
         "url": "https://x.com/settings/location"
      } ],
      "current_task": null,
      "errors": {
      },
      "progress": $1,
      "start_url": "https://x.com/home",
      "state": "started",
      "tasks_list": [  ]
   },
   "result": $2
})";

constexpr double kProgress{5};

constexpr char kNoPsstDictItemPolicyScriptScriptResult[] = R"({
  "someitem": "val"
})";

const std::vector<std::string> kUrlsToSkip =
    std::vector<std::string>({"http://url1.com", "http://url2.com"});

}  // namespace
namespace psst {

class MockPsstRuleRegistryImpl : public PsstRuleRegistryImpl {
 public:
  MOCK_METHOD(void, OnLoadRules, (const std::string& data), (override));
};

class MockPsstDialogDelegate : public PsstDialogDelegate {
 public:
  MockPsstDialogDelegate() = default;
  ~MockPsstDialogDelegate() override = default;
  MOCK_METHOD(void, Show, (), (override));
  MOCK_METHOD(void, SetProgressValue, (const double value), (override));
  MOCK_METHOD(void,
              SetCompletedView,
              (const std::optional<std::vector<std::string>>& applied_checks,
               const std::optional<std::vector<std::string>>&
                   errors  //, ShareCallback share_cb
               ),
              (override));
  MOCK_METHOD(void, Close, (), (override));
};

class MockHandlerForProcessUserScriptImpl : public PsstScriptsHandlerImpl {
 public:
  explicit MockHandlerForProcessUserScriptImpl(
      std::unique_ptr<PsstDialogDelegate> delegate,
      PrefService* prefs,
      content::WebContents* web_contents,
      content::RenderFrameHost* const render_frame_host)
      : PsstScriptsHandlerImpl(std::move(delegate),
                               prefs,
                               web_contents,
                               render_frame_host,
                               0) {}

  MOCK_METHOD(void, ResetContext, (), (override));
  MOCK_METHOD(void,
              OnUserDialogAction,
              (const bool is_initial,
               const std::string& user_id,
               const MatchedRule& rule,
               std::optional<base::Value> script_params,
               const PsstConsentStatus status,
               const std::vector<std::string>& disabled_checks),
              (override));
  MOCK_METHOD(void,
              InsertScriptInPage,
              (const std::string& script,
               std::optional<base::Value> value,
               InsertScriptInPageCallback cb),
              (override));
};
class MockHandlerForProcessUserDialogActionImpl
    : public PsstScriptsHandlerImpl {
 public:
  explicit MockHandlerForProcessUserDialogActionImpl(
      std::unique_ptr<PsstDialogDelegate> delegate,
      PrefService* prefs,
      content::WebContents* web_contents,
      content::RenderFrameHost* const render_frame_host)
      : PsstScriptsHandlerImpl(std::move(delegate),
                               prefs,
                               web_contents,
                               render_frame_host,
                               0) {}

  MOCK_METHOD(void, ResetContext, (), (override));
  MOCK_METHOD(void,
              InsertScriptInPage,
              (const std::string& script,
               std::optional<base::Value> value,
               InsertScriptInPageCallback cb),
              (override));
};

class MockHandlerPolicyScriptResultImpl : public PsstScriptsHandlerImpl {
 public:
  explicit MockHandlerPolicyScriptResultImpl(
      std::unique_ptr<PsstDialogDelegate> delegate,
      PrefService* prefs,
      content::WebContents* web_contents,
      content::RenderFrameHost* const render_frame_host)
      : PsstScriptsHandlerImpl(std::move(delegate),
                               prefs,
                               web_contents,
                               render_frame_host,
                               0) {}
  MOCK_METHOD(void, ResetContext, (), (override));
};

class PsstScriptsHandlerUnitTest : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    feature_list_.InitAndEnableFeature(psst::features::kBravePsst);
    psst::RegisterProfilePrefs(pref_service_.registry());

    base::FilePath test_data_dir(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));
    test_data_dir_base_ = test_data_dir.AppendASCII("psst-component-data");

    auto rule_data_reader =
        std::make_unique<RuleDataReader>(base::FilePath(test_data_dir_base_));
    rule_data_reader_ = rule_data_reader.get();

    auto psst_rul_registry = std::make_unique<MockPsstRuleRegistryImpl>();
    psst_rul_registry_ = psst_rul_registry.get();

    PsstRuleRegistryAccessor::GetInstance()->SetRegistryForTesting(
        std::move(psst_rul_registry));

    psst_rul_registry_->SetRuleDataReaderForTest(std::move(rule_data_reader));
  }

  content::RenderFrameHostTester* render_frame_host_tester(
      content::RenderFrameHost* host) {
    return content::RenderFrameHostTester::For(host);
  }

  base::FilePath GetTestDataDirBase() const { return test_data_dir_base_; }

  PrefService* GetPrefs() { return &pref_service_; }

  MockPsstRuleRegistryImpl* GetPsstRuleRegistry() { return psst_rul_registry_; }

  void LoadRules() {
    base::RunLoop run_loop;
    EXPECT_CALL(*GetPsstRuleRegistry(), OnLoadRules(testing::_))
        .WillOnce(testing::Invoke([&](const std::string& data) {
          EXPECT_EQ(data, ReadFile(GetTestDataDirBase().Append(
                              base::FilePath::FromUTF8Unsafe("psst.json"))));
          (*GetPsstRuleRegistry()).PsstRuleRegistryImpl::OnLoadRules(data);
          run_loop.Quit();
        }));
    PsstRuleRegistryAccessor::GetInstance()->Registry()->LoadRules(
        GetTestDataDirBase());
    run_loop.Run();
  }

  void ProcessUserScriptHandlerTest(
      const GURL& url,
      base::Value script_result,
      base::OnceCallback<void(MockHandlerForProcessUserScriptImpl& handler)>
          setup_mock_callback) {
    LoadRules();
    base::RunLoop run_loop;
    MockHandlerForProcessUserScriptImpl handler(
        std::make_unique<MockPsstDialogDelegate>(), GetPrefs(), web_contents(),
        main_rfh());
    std::move(setup_mock_callback).Run(handler);

    base::MockCallback<
        base::OnceCallback<void(const std::optional<MatchedRule>&)>>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly([&](const std::optional<MatchedRule>& matched_rule) {
          EXPECT_TRUE(matched_rule.has_value());
          handler.OnUserScriptResult(matched_rule.value(),
                                     std::move(script_result));
          run_loop.Quit();
        });

    PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
        url, mock_callback.Get());

    run_loop.Run();
  }

  void ProcessUserDialogActionTest(
      const GURL& url,
      base::Value script_result,
      const bool is_initial,
      const std::string& user_id,
      const PsstConsentStatus status,
      const std::vector<std::string>& disabled_checks,
      base::OnceCallback<void(MockHandlerForProcessUserDialogActionImpl&
                                  handler)> setup_mock_callback) {
    LoadRules();
    base::RunLoop run_loop;
    MockHandlerForProcessUserDialogActionImpl handler(
        std::make_unique<MockPsstDialogDelegate>(), GetPrefs(), web_contents(),
        main_rfh());
    std::move(setup_mock_callback).Run(handler);

    base::MockCallback<
        base::OnceCallback<void(const std::optional<MatchedRule>&)>>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly([&](const std::optional<MatchedRule>& matched_rule) {
          EXPECT_TRUE(matched_rule.has_value());
          handler.OnUserDialogAction(is_initial, user_id, matched_rule.value(),
                                     std::move(script_result), status,
                                     disabled_checks);

          const auto settings_for_site =
              GetPsstSettings(user_id, matched_rule->Name(), GetPrefs());

          EXPECT_TRUE(settings_for_site.has_value());
          EXPECT_EQ(status, settings_for_site->consent_status);
          EXPECT_EQ(disabled_checks, settings_for_site->urls_to_skip);

          run_loop.Quit();
        });

    PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
        url, mock_callback.Get());

    run_loop.Run();
  }

  void ProcessPolicyScriptResult(
      const GURL& url,
      base::Value script_result,
      const double progress,
      base::OnceCallback<void(MockHandlerPolicyScriptResultImpl& handler)>
          setup_mock_callback) {
    LoadRules();
    base::RunLoop run_loop;
    MockHandlerPolicyScriptResultImpl handler(
        std::make_unique<MockPsstDialogDelegate>(), GetPrefs(), web_contents(),
        main_rfh());
    std::move(setup_mock_callback).Run(handler);

    base::MockCallback<
        base::OnceCallback<void(const std::optional<MatchedRule>&)>>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly([&](const std::optional<MatchedRule>& matched_rule) {
          EXPECT_TRUE(matched_rule.has_value());
          handler.OnPolicyScriptResult(matched_rule.value(),
                                       std::move(script_result));
          run_loop.Quit();
        });

    PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
        url, mock_callback.Get());

    run_loop.Run();
  }

  void ProcessInsertPolicyScriptNoRule() {
    MockHandlerForProcessUserDialogActionImpl handler(
        std::make_unique<MockPsstDialogDelegate>(), GetPrefs(), web_contents(),
        main_rfh());
    EXPECT_CALL(handler, InsertScriptInPage).Times(0);
    EXPECT_CALL(handler, ResetContext()).Times(1);
    handler.InsertPolicyScript(std::nullopt);
  }

  void ProcessInsertPolicyScript(
      const GURL& url,
      base::Value script_result,
      base::OnceCallback<void(MockHandlerForProcessUserDialogActionImpl&
                                  handler)> setup_mock_callback) {
    LoadRules();
    base::RunLoop run_loop;
    MockHandlerForProcessUserDialogActionImpl handler(
        std::make_unique<MockPsstDialogDelegate>(), GetPrefs(), web_contents(),
        main_rfh());
    std::move(setup_mock_callback).Run(handler);

    base::MockCallback<
        base::OnceCallback<void(const std::optional<MatchedRule>&)>>
        mock_callback;
    EXPECT_CALL(mock_callback, Run)
        .Times(1)
        .WillRepeatedly([&](const std::optional<MatchedRule>& matched_rule) {
          EXPECT_TRUE(matched_rule.has_value());
          handler.TryToLoadContext(matched_rule.value(), script_result);
          handler.InsertPolicyScript(matched_rule.value());
          run_loop.Quit();
        });

    PsstRuleRegistryAccessor::GetInstance()->Registry()->CheckIfMatch(
        url, mock_callback.Get());

    run_loop.Run();
  }

  raw_ptr<RuleDataReader> rule_data_reader_;
  base::FilePath test_data_dir_base_;
  raw_ptr<MockPsstRuleRegistryImpl> psst_rul_registry_{nullptr};

  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(PsstScriptsHandlerUnitTest, HandleUserScriptResult) {
  ProcessUserScriptHandlerTest(
      GURL("https://a.com"), base::Value(),
      base::BindOnce([](MockHandlerForProcessUserScriptImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
      }));

  ProcessUserScriptHandlerTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kNoUserPropUserScriptResult, {kShareExperienceLink, kName}, nullptr)),
      base::BindOnce([](MockHandlerForProcessUserScriptImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
      }));

  SetPsstSettings(kUserId, kName, PsstSettings{psst::kBlock, 1}, GetPrefs());
  ProcessUserScriptHandlerTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kCorrectUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      base::BindOnce([](MockHandlerForProcessUserScriptImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
      }));

  SetPsstSettings(kUserId, kName, PsstSettings{psst::kAllow, 1, kUrlsToSkip},
                  GetPrefs());
  ProcessUserScriptHandlerTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kCorrectUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      base::BindOnce([](MockHandlerForProcessUserScriptImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(0);
        EXPECT_CALL(handler,
                    OnUserDialogAction(false, kUserId, testing::_, testing::_,
                                       kAllow, kUrlsToSkip))
            .Times(1);
      }));

  SetPsstSettings(kUserId, kName, PsstSettings{psst::kBlock, 1}, GetPrefs());
  ProcessUserScriptHandlerTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kNoTasksUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      base::BindOnce([](MockHandlerForProcessUserScriptImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
      }));
  ProcessUserScriptHandlerTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kEmptyTasksUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      base::BindOnce([](MockHandlerForProcessUserScriptImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
      }));
  ProcessUserScriptHandlerTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kNoNameUserScriptResult, {kUserId, kShareExperienceLink}, nullptr)),
      base::BindOnce([](MockHandlerForProcessUserScriptImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
      }));
  SetPsstSettings(kUserId, kName, PsstSettings{psst::kAsk, 1}, GetPrefs());
  ProcessUserScriptHandlerTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kCorrectUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      base::BindOnce([](MockHandlerForProcessUserScriptImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(0);
        auto* delegate = static_cast<MockPsstDialogDelegate*>(
            handler.GetPsstDialogDelegate());
        EXPECT_CALL(*delegate, Show)
            .Times(1)
            .WillOnce(testing::Invoke([delegate]() {
              const auto* show_dialog_data = delegate->GetShowDialogData();
              EXPECT_TRUE(show_dialog_data);
              EXPECT_EQ(false, show_dialog_data->is_new_version);
              EXPECT_EQ(kName, show_dialog_data->site_name);
              EXPECT_EQ(1u, show_dialog_data->request_infos.size());
            }));
      }));
  SetPsstSettings(kUserId, kName, PsstSettings{psst::kAllow, 0}, GetPrefs());
  ProcessUserScriptHandlerTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kCorrectUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      base::BindOnce([](MockHandlerForProcessUserScriptImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(0);
        auto* delegate = static_cast<MockPsstDialogDelegate*>(
            handler.GetPsstDialogDelegate());
        EXPECT_CALL(*delegate, Show)
            .Times(1)
            .WillOnce(testing::Invoke([delegate]() {
              const auto* show_dialog_data = delegate->GetShowDialogData();
              EXPECT_TRUE(show_dialog_data);
              EXPECT_EQ(true, show_dialog_data->is_new_version);
              EXPECT_EQ(kName, show_dialog_data->site_name);
              EXPECT_EQ(1u, show_dialog_data->request_infos.size());
            }));
      }));
}

TEST_F(PsstScriptsHandlerUnitTest, HandleUserDialogAction) {
  ProcessUserDialogActionTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kCorrectUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      true, kUserId, PsstConsentStatus::kAllow, std::vector<std::string>{""},
      base::BindOnce([](MockHandlerForProcessUserDialogActionImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(0);
        EXPECT_CALL(handler, InsertScriptInPage)
            .Times(1)
            .WillRepeatedly([](const std::string& script,
                               std::optional<base::Value> value,
                               MockHandlerForProcessUserScriptImpl::
                                   InsertScriptInPageCallback cb) {
              EXPECT_FALSE(script.empty());
              auto* tasks =
                  value->GetDict().FindList(kUserScriptResultTasksPropName);
              EXPECT_TRUE(tasks);
              EXPECT_EQ(1u, tasks->size());
            });
      }));

  ProcessUserDialogActionTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kCorrectUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      true, kUserId, PsstConsentStatus::kAllow,
      std::vector<std::string>{"https://x.com/settings/ads_preferences"},
      base::BindOnce([](MockHandlerForProcessUserDialogActionImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(0);
        EXPECT_CALL(handler, InsertScriptInPage)
            .Times(1)
            .WillRepeatedly([](const std::string& script,
                               std::optional<base::Value> value,
                               MockHandlerForProcessUserScriptImpl::
                                   InsertScriptInPageCallback cb) {
              EXPECT_FALSE(script.empty());
              auto* tasks =
                  value->GetDict().FindList(kUserScriptResultTasksPropName);
              EXPECT_TRUE(tasks);
              EXPECT_EQ(0u, tasks->size());
            });
      }));

  ProcessUserDialogActionTest(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kCorrectUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      true, kUserId, PsstConsentStatus::kBlock, std::vector<std::string>{""},
      base::BindOnce([](MockHandlerForProcessUserDialogActionImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(0);
        EXPECT_CALL(handler, InsertScriptInPage).Times(0);
      }));
}

TEST_F(PsstScriptsHandlerUnitTest, HandlePolicyScriptResult) {
  const bool result{true};
  ProcessPolicyScriptResult(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kPolicyScriptScriptResult,
          {base::ToString(kProgress), base::ToString(result)}, nullptr)),
      kProgress, base::BindOnce([](MockHandlerPolicyScriptResultImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
        EXPECT_CALL(*(static_cast<MockPsstDialogDelegate*>(
                        handler.GetPsstDialogDelegate())),
                    SetProgressValue(kProgress))
            .Times(1);

        EXPECT_CALL(*(static_cast<MockPsstDialogDelegate*>(
                        handler.GetPsstDialogDelegate())),
                    SetCompletedView)
            .Times(1)
            .WillRepeatedly(
                [](const std::optional<std::vector<std::string>>&
                       applied_checks,
                   const std::optional<std::vector<std::string>>&
                       errors  //, MockPsstDialogDelegate::ShareCallback
                               // share_cb
                ) {
                  EXPECT_TRUE(applied_checks.has_value());
                  EXPECT_FALSE(errors.has_value());
                  EXPECT_EQ(2u, applied_checks->size());
                });
      }));

  ProcessPolicyScriptResult(
      GURL("https://a.com"), base::Value(), kProgress,
      base::BindOnce([](MockHandlerPolicyScriptResultImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
        EXPECT_CALL(*(static_cast<MockPsstDialogDelegate*>(
                        handler.GetPsstDialogDelegate())),
                    SetCompletedView)
            .Times(0);
      }));

  ProcessPolicyScriptResult(
      GURL("https://a.com"), ParseJson(kNoPsstDictItemPolicyScriptScriptResult),
      kProgress, base::BindOnce([](MockHandlerPolicyScriptResultImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
        EXPECT_CALL(*(static_cast<MockPsstDialogDelegate*>(
                        handler.GetPsstDialogDelegate())),
                    SetCompletedView)
            .Times(0);
      }));

  ProcessPolicyScriptResult(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kPolicyScriptScriptResult,
          {base::ToString(kProgress), base::ToString(false)}, nullptr)),
      kProgress, base::BindOnce([](MockHandlerPolicyScriptResultImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(0);
        EXPECT_CALL(*(static_cast<MockPsstDialogDelegate*>(
                        handler.GetPsstDialogDelegate())),
                    SetProgressValue(kProgress))
            .Times(1);
        EXPECT_CALL(*(static_cast<MockPsstDialogDelegate*>(
                        handler.GetPsstDialogDelegate())),
                    SetCompletedView)
            .Times(0);
      }));
}

TEST_F(PsstScriptsHandlerUnitTest, HandleInsertPolicyScript) {
  ProcessInsertPolicyScriptNoRule();

  ProcessInsertPolicyScript(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kCorrectUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      base::BindOnce([](MockHandlerForProcessUserDialogActionImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(1);
      }));

  SetPsstSettings(kUserId, kName,
                  PsstSettings{PsstConsentStatus::kAllow, 1, {}}, GetPrefs());
  ProcessInsertPolicyScript(
      GURL("https://a.com"),
      ParseJson(base::ReplaceStringPlaceholders(
          kCorrectUserScriptResult, {kUserId, kShareExperienceLink, kName},
          nullptr)),
      base::BindOnce([](MockHandlerForProcessUserDialogActionImpl& handler) {
        EXPECT_CALL(handler, ResetContext()).Times(0);
        EXPECT_CALL(handler, InsertScriptInPage)
            .Times(1)
            .WillRepeatedly([](const std::string& script,
                               std::optional<base::Value> value,
                               MockHandlerForProcessUserScriptImpl::
                                   InsertScriptInPageCallback cb) {
              EXPECT_FALSE(script.empty());
              auto initial_execution = value->GetDict().FindBool(
                  kUserScriptResultInitialExecutionPropName);
              EXPECT_TRUE(initial_execution.has_value());
              EXPECT_EQ(false, initial_execution.value());
            });
      }));
}

}  // namespace psst

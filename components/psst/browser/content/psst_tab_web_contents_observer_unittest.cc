// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "brave/components/psst/common/psst_script_responses.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::InvokeArgument;

namespace psst {

class DocumentOnLoadObserver : public content::WebContentsObserver {
 public:
  explicit DocumentOnLoadObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}
  void Wait() { loop_.Run(); }

 private:
  void DocumentOnLoadCompletedInPrimaryMainFrame() override { loop_.Quit(); }

  base::RunLoop loop_;
};

class MockPsstRuleRegistry final : public PsstRuleRegistry {
 public:
  MockPsstRuleRegistry() = default;
  ~MockPsstRuleRegistry() = default;

  MOCK_METHOD(void,
              CheckIfMatch,
              (const GURL&,
               base::OnceCallback<void(std::unique_ptr<MatchedRule>)>),
              (override));

  MOCK_METHOD(void,
              LoadRules,
              (const base::FilePath& path, PsstRuleRegistry::OnLoadCallback cb),
              (override));
};

// testing::InvokeArgument<N> does not work with base::OnceCallback, so we
// define our own gMock action to run the 2nd argument.
ACTION_P(CheckIfMatchCallback, loop, match_rule) {
  std::move(
      const_cast<base::OnceCallback<void(std::unique_ptr<MatchedRule>)>&>(arg1))
      .Run(base::WrapUnique(match_rule));
  loop->Quit();
}

ACTION_P(CheckIfMatchFailsCallback, loop) {
  std::move(
      const_cast<base::OnceCallback<void(std::unique_ptr<MatchedRule>)>&>(arg1))
      .Run(nullptr);
  loop->Quit();
}

// testing::InvokeArgument<N> does not work with base::OnceCallback, so we
// define our own gMock action to run the 2nd argument.
ACTION_P(InsertScriptInPageCallback, future, value) {
  std::move(
      const_cast<PsstTabWebContentsObserver::InsertScriptInPageCallback&>(arg1))
      .Run(value.Clone());
  future->SetValue(value.Clone());
}

class MockUiDelegate : public PsstTabWebContentsObserver::PsstUiDelegate {
 public:
  MockUiDelegate() = default;
  ~MockUiDelegate() override = default;

  MOCK_METHOD(void,
              SetAppliedItems,
              (long progress, const std::vector<PsstTask>& applied_tasks),
              (override));
  MOCK_METHOD(void, SetComplete, (), (override));
};

class PsstTabWebContentsObserverUnitTestBase
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    psst::RegisterProfilePrefs(prefs_.registry());
    rule_registry_ = std::make_unique<MockPsstRuleRegistry>();

    auto ui_delegate = std::make_unique<MockUiDelegate>();
    ui_delegate_ = ui_delegate.get();

    psst_web_contents_observer_ = base::WrapUnique<PsstTabWebContentsObserver>(
        new PsstTabWebContentsObserver(web_contents(), rule_registry_.get(),
                                       &prefs_, std::move(ui_delegate),
                                       inject_script_callback_.Get()));
  }

  void TearDown() override {
    content::RenderViewHostTestHarness::TearDown();
    ui_delegate_ = nullptr;
  }

  MockPsstRuleRegistry& psst_rule_registry() { return *rule_registry_.get(); }
  PrefService* prefs() { return &prefs_; }

  MatchedRule* CreateMatchedRule(const std::string& user_script,
                                 const std::string& policy_script) {
    return new MatchedRule("name", user_script, policy_script, 1);
  }

  base::MockCallback<PsstTabWebContentsObserver::InjectScriptCallback>&
  inject_script_callback() {
    return inject_script_callback_;
  }

  MockUiDelegate& ui_delegate() { return *ui_delegate_; }

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  raw_ptr<MockUiDelegate> ui_delegate_;
  base::MockCallback<PsstTabWebContentsObserver::InjectScriptCallback>
      inject_script_callback_;
  std::unique_ptr<MockPsstRuleRegistry> rule_registry_;
  std::unique_ptr<PsstTabWebContentsObserver> psst_web_contents_observer_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
};

class PsstTabWebContentsObserverUnitTest
    : public PsstTabWebContentsObserverUnitTestBase {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(psst::features::kEnablePsst);
    PsstTabWebContentsObserverUnitTestBase::SetUp();
  }
};

TEST_F(PsstTabWebContentsObserverUnitTest, CreateForRegularBrowserContext) {
  EXPECT_NE(PsstTabWebContentsObserver::MaybeCreateForWebContents(
                web_contents(), browser_context(),
                std::make_unique<MockUiDelegate>(), prefs(), 2),
            nullptr);
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       DontCreateForIncognitoBrowserContext) {
  content::TestBrowserContext otr_browser_context;
  otr_browser_context.set_is_off_the_record(true);
  auto site_instance = content::SiteInstance::Create(&otr_browser_context);
  auto web_contents = content::WebContentsTester::CreateTestWebContents(
      &otr_browser_context, site_instance);

  EXPECT_EQ(PsstTabWebContentsObserver::MaybeCreateForWebContents(
                web_contents.get(), &otr_browser_context,
                std::make_unique<MockUiDelegate>(), prefs(), 2),
            nullptr);
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldNotProcessRestoredNavigationEntry) {
  auto url = GURL("https://example1.com");

  content::NavigationController& controller = web_contents()->GetController();
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch).Times(0);

  std::unique_ptr<content::NavigationEntry> restored_entry =
      content::NavigationEntry::Create();
  restored_entry->SetURL(url);
  restored_entry->SetTitle(u"Restored Page");

  std::vector<std::unique_ptr<content::NavigationEntry>> entries;
  entries.push_back(std::move(restored_entry));

  DocumentOnLoadObserver observer(web_contents());
  controller.Restore(0 /* selected_index */, content::RestoreType::kRestored,
                     &entries);

  controller.LoadIfNecessary();

  auto navigation_simulator =
      content::NavigationSimulator::CreateFromPending(controller);
  navigation_simulator->Commit();
  observer.Wait();
}

TEST_F(PsstTabWebContentsObserverUnitTest, ShouldOnlyProcessHttpOrHttps) {
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(_, _)).Times(0);
  {
    DocumentOnLoadObserver observer(web_contents());
    content::NavigationSimulator::NavigateAndCommitFromBrowser(
        web_contents(), GURL("chrome://flags"));
    observer.Wait();
  }
  {
    DocumentOnLoadObserver observer(web_contents());
    content::NavigationSimulator::NavigateAndCommitFromBrowser(
        web_contents(), GURL("about:blank"));
    observer.Wait();
  }
  {
    DocumentOnLoadObserver observer(web_contents());
    content::NavigationSimulator::NavigateAndCommitFromBrowser(
        web_contents(), GURL("file:///somepath"));
    observer.Wait();
  }
  {
    DocumentOnLoadObserver observer(web_contents());
    content::NavigationSimulator::NavigateAndCommitFromBrowser(
        web_contents(), GURL("ftp://example.com"));
    observer.Wait();
  }
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldProcessMultipleMainFrameNavigations) {
  const std::string first_nav_user_script = "user1";
  const std::string policy_script = "policy";
  const GURL first_navigation_url("https://example1.com");

  base::RunLoop first_nav_check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(first_navigation_url, _))
      .WillOnce(CheckIfMatchCallback(
          &first_nav_check_loop,
          CreateMatchedRule(first_nav_user_script, policy_script)));

  base::test::TestFuture<base::Value> first_nav_user_script_insert_future;
  EXPECT_CALL(inject_script_callback(), Run(first_nav_user_script, _))
      .WillOnce(InsertScriptInPageCallback(&first_nav_user_script_insert_future,
                                           base::Value()));

  DocumentOnLoadObserver first_nav_observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), first_navigation_url);
  first_nav_observer.Wait();

  first_nav_check_loop.Run();
  EXPECT_EQ(base::Value(), first_nav_user_script_insert_future.Take());

  const std::string second_nav_user_script = "user2";
  const GURL second_navigation_url("https://example2.com");
  base::RunLoop second_nav_check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(second_navigation_url, _))
      .WillOnce(CheckIfMatchCallback(
          &second_nav_check_loop,
          CreateMatchedRule(second_nav_user_script, policy_script)));

  base::test::TestFuture<base::Value> second_nav_user_script_insert_future;
  EXPECT_CALL(inject_script_callback(), Run(second_nav_user_script, _))
      .WillOnce(InsertScriptInPageCallback(
          &second_nav_user_script_insert_future, base::Value()));
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), second_navigation_url);
  observer.Wait();

  second_nav_check_loop.Run();
  EXPECT_EQ(base::Value(), second_nav_user_script_insert_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest, ShouldProcessRedirectsNavigations) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  const GURL redirect_target("https://redirect.example1.com/");

  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _)).Times(0);
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(redirect_target, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));

  base::test::TestFuture<base::Value> user_script_insert_future;
  auto value = base::Value();
  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           std::move(value)));
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  auto simulator =
      content::NavigationSimulator::CreateBrowserInitiated(url, web_contents());
  simulator->Redirect(redirect_target);
  simulator->Commit();

  observer.Wait();
  check_loop.Run();
  EXPECT_EQ(base::Value(), user_script_insert_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldNotProcessIfNotPrimaryMainFrame) {
  // first load the main frame so we can create a subframe
  GURL url("https://example.com/");

  // call one for the main frame and then not again
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _)).Times(1);
  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  // create sub-frame
  auto* main_rfh = web_contents()->GetPrimaryMainFrame();
  auto* child_rfh =
      content::RenderFrameHostTester::For(main_rfh)->AppendChild("subframe");

  // navigate sub-frame
  content::NavigationSimulator::CreateRendererInitiated(
      GURL("https://sub.example.com"), child_rfh)
      ->Commit();
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldNotProcessIfNavigationNotCommitted) {
  const GURL url("https://example.com");
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _)).Times(0);
  auto simulator =
      content::NavigationSimulator::CreateBrowserInitiated(url, web_contents());

  // Simulate navigation start but NOT commit
  simulator->Start();
  simulator->Fail(net::ERR_ABORTED);  // Simulates cancel before commit
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldNotProcessIfSameDocumentNavigation) {
  const GURL url("https://example1.com");
  // should call once for the initial load and then not again
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _)).Times(1);
  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  auto sim = content::NavigationSimulator::CreateRendererInitiated(
      GURL(base::JoinString({url.spec(), "anchor"}, "#")),
      web_contents()->GetPrimaryMainFrame());
  sim->CommitSameDocument();
}

TEST_F(PsstTabWebContentsObserverUnitTest, DefaultPrefEnabledShouldProcess) {
  const GURL url("https://example1.com");
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _)).Times(1);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();
}

TEST_F(PsstTabWebContentsObserverUnitTest, PrefDisabledDontProcess) {
  const GURL url("https://example1.com");
  prefs()->SetBoolean(prefs::kPsstEnabled, false);
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _)).Times(0);
  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();
}

TEST_F(PsstTabWebContentsObserverUnitTest, CheckIfMatchReturnsNull) {
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchFailsCallback(&check_loop));
  EXPECT_CALL(inject_script_callback(), Run).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsEmptyNoPolicyScript) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::test::TestFuture<base::Value> user_script_insert_future;

  // User script result is not a dictionary
  auto script_params = base::Value();

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           std::move(script_params)));
  // No policy script executed
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();
  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsDictEmptyPolicyScript) {
  const std::string user_script = "user";
  // Policy script is empty
  const std::string policy_script = "";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::test::TestFuture<base::Value> user_script_insert_future;
  auto script_params = base::Value(base::Value::Dict());

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           std::move(script_params)));
  // No policy script executed
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptNotReturnUserNoPolicyScript) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::test::TestFuture<base::Value> user_script_insert_future;

  // User script result is an empty dictionary
  auto script_params = base::Value(base::Value::Dict());

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           std::move(script_params)));
  // No policy script executed
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsUserHasPolicyScript) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::test::TestFuture<base::Value> user_script_insert_future;
  base::test::TestFuture<base::Value> policy_script_insert_future;

  // User script result is an dictionary, and user key is not empty
  auto script_params = base::Value(base::Value::Dict().Set("user", "value"));

  // Policy script result is a dictionary, but it is not deserializable
  auto policy_script_result =
      base::Value(base::Value::Dict().Set("prop", "value"));

  // Any UI delegate method must not be called, as policy_script_result is empty
  EXPECT_CALL(ui_delegate(), SetAppliedItems(_, _)).Times(0);
  EXPECT_CALL(ui_delegate(), SetComplete()).Times(0);

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           script_params.Clone()));

  const auto script_with_parameters = base::StrCat(
      {"const params = ",
       base::WriteJsonWithOptions(script_params.Clone(),
                                  base::JSONWriter::OPTIONS_PRETTY_PRINT)
           .value(),
       ";\n", policy_script});

  // Policy script executed, parameters added
  EXPECT_CALL(inject_script_callback(), Run(script_with_parameters, _))
      .WillOnce(InsertScriptInPageCallback(&policy_script_insert_future,
                                           policy_script_result.Clone()));

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
  EXPECT_EQ(policy_script_result, policy_script_insert_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsEmptyUserNoPolicyScript) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::test::TestFuture<base::Value> user_script_insert_future;

  // User script result is an dictionary, but user key is empty
  auto script_params = base::Value(base::Value::Dict().Set("user", ""));

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           script_params.Clone()));
  // No policy script executed
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsUnsupportedDictNoPolicyScriptParams) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));

  base::test::TestFuture<base::Value> user_script_insert_future;
  base::test::TestFuture<base::Value> policy_script_insert_future;

  // Create a dictionary with unsupported blob storage value
  auto script_params = base::Value(
      base::Value::Dict()
          .Set("user", "value")
          .Set("prop",
               base::Value(base::Value::BlobStorage{0x01, 0x02, 0x03})));
  auto policy_script_result = base::Value();

  // Any UI delegate method must not be called, as policy_script_result is empty
  EXPECT_CALL(ui_delegate(), SetAppliedItems(_, _)).Times(0);
  EXPECT_CALL(ui_delegate(), SetComplete()).Times(0);

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           script_params.Clone()));
  // Policy script executed, parameters not added
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _))
      .WillOnce(InsertScriptInPageCallback(&policy_script_insert_future,
                                           policy_script_result.Clone()));

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
  EXPECT_EQ(policy_script_result, policy_script_insert_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UiDelegateSetAppliedItemsButNotSetComplete) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  const int progress = 50;
  base::RunLoop check_loop;
  base::test::TestFuture<long> progress_future;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));

  // SetAppliedItems must be called as policy script returns valid result with
  // STARTED status
  EXPECT_CALL(ui_delegate(), SetAppliedItems(progress, _))
      .WillOnce([&progress_future](long progress,
                                   const std::vector<PsstTask>& applied_tasks) {
        progress_future.SetValue(progress);
      });
  // SetComplete must not be called as policy script returns STARTED status
  EXPECT_CALL(ui_delegate(), SetComplete()).Times(0);

  base::test::TestFuture<base::Value> user_script_insert_future;
  base::test::TestFuture<base::Value> policy_script_insert_future;

  // Create a user script return value
  auto script_params = base::Value(base::Value::Dict().Set("user", "value"));

  // prepare return value for policy script (status should be STARTED)
  auto policy_script_result =
      base::Value(base::Value::Dict()
                      .Set("status", "STARTED")
                      .Set("progress", progress)
                      .Set("applied_tasks",
                           base::Value::List().Append(
                               base::Value::Dict()
                                   .Set("url", "https://example1.com/task")
                                   .Set("description", "Ads Preferences"))));

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           script_params.Clone()));

  const auto policy_script_with_parameters = base::StrCat(
      {"const params = ",
       base::WriteJsonWithOptions(script_params.Clone(),
                                  base::JSONWriter::OPTIONS_PRETTY_PRINT)
           .value(),
       ";\n", policy_script});
  // Policy script executed, parameters added
  EXPECT_CALL(inject_script_callback(), Run(policy_script_with_parameters, _))
      .WillOnce(InsertScriptInPageCallback(&policy_script_insert_future,
                                           policy_script_result.Clone()));

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
  EXPECT_EQ(policy_script_result, policy_script_insert_future.Take());
  EXPECT_EQ(progress, progress_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UiDelegateSetAppliedItemsButNotSetCompleteMultipleTasks) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const std::string second_policy_script = "second_policy";
  const GURL url("https://example1.com");
  const GURL second_url("https://example1.com/second");
  const int first_progress_value = 50;
  const int second_progress_value = 60;
  const std::string task_description = "first task desription";
  const std::string second_task_description = "second task description";
  // Create a user script return value
  auto script_params = base::Value(base::Value::Dict().Set("user", "value"));

  // prepare return value for policy script (status should be STARTED)
  auto policy_script_result =
      base::Value(base::Value::Dict()
                      .Set("status", "STARTED")
                      .Set("progress", first_progress_value)
                      .Set("applied_tasks",
                           base::Value::List().Append(
                               base::Value::Dict()
                                   .Set("url", url.spec())
                                   .Set("description", task_description))));

  base::RunLoop check_loop;
  base::RunLoop second_navigation_check_loop;
  base::test::TestFuture<long> progress_future;
  base::test::TestFuture<std::vector<PsstTask>> applied_tasks_future;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(second_url, _))
      .WillOnce(CheckIfMatchCallback(
          &second_navigation_check_loop,
          CreateMatchedRule(user_script, second_policy_script)));

  // SetAppliedItems must be called as policy script returns valid result with
  // STARTED status
  EXPECT_CALL(ui_delegate(), SetAppliedItems(_, _))
      .Times(2)
      .WillRepeatedly([&progress_future, &applied_tasks_future](
                          long progress,
                          const std::vector<PsstTask>& applied_tasks) {
        progress_future.SetValue(progress);
        std::vector<PsstTask> tasks;
        std::ranges::for_each(applied_tasks, [&tasks](const PsstTask& task) {
          tasks.push_back(task.Clone());
        });
        applied_tasks_future.SetValue(std::move(tasks));
      });

  // SetComplete must not be called as policy script returns STARTED status
  EXPECT_CALL(ui_delegate(), SetComplete()).Times(0);

  base::test::TestFuture<base::Value> user_script_insert_future;
  base::test::TestFuture<base::Value> policy_script_insert_future;

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .Times(2)
      .WillRepeatedly(InsertScriptInPageCallback(&user_script_insert_future,
                                                 script_params.Clone()));

  const auto policy_script_with_parameters =
      [&script_params](const std::string& policy_script) {
        return base::StrCat(
            {"const params = ",
             base::WriteJsonWithOptions(script_params.Clone(),
                                        base::JSONWriter::OPTIONS_PRETTY_PRINT)
                 .value(),
             ";\n", policy_script});
      };
  // Policy script executed, parameters added
  EXPECT_CALL(inject_script_callback(),
              Run(policy_script_with_parameters(policy_script), _))
      .WillOnce(InsertScriptInPageCallback(&policy_script_insert_future,
                                           policy_script_result.Clone()));

  // Prepare policy script results for the second navigation
  auto second_policy_script_result = policy_script_result.Clone();
  second_policy_script_result.GetDict().Set("progress", second_progress_value);
  auto* second_applied_tasks_list =
      second_policy_script_result.GetDict().FindList("applied_tasks");
  ASSERT_TRUE(second_applied_tasks_list);
  second_applied_tasks_list->begin()->GetDict().Set("url", second_url.spec());
  second_applied_tasks_list->begin()->GetDict().Set("description",
                                                    second_task_description);

  EXPECT_CALL(inject_script_callback(),
              Run(policy_script_with_parameters(second_policy_script), _))
      .WillOnce(InsertScriptInPageCallback(
          &policy_script_insert_future, second_policy_script_result.Clone()));

  // First navigation (task)
  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
  user_script_insert_future.Clear();

  EXPECT_EQ(policy_script_result, policy_script_insert_future.Take());
  policy_script_insert_future.Clear();

  EXPECT_EQ(first_progress_value, progress_future.Take());
  progress_future.Clear();

  const auto& applied_tasks = applied_tasks_future.Take();
  EXPECT_EQ(1u, applied_tasks.size());
  EXPECT_EQ(url.spec(), applied_tasks[0].url);
  EXPECT_EQ(task_description, applied_tasks[0].description);
  applied_tasks_future.Clear();

  // Second navigation (task)
  DocumentOnLoadObserver second_navigation_observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             second_url);
  second_navigation_observer.Wait();

  second_navigation_check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
  EXPECT_EQ(second_policy_script_result, policy_script_insert_future.Take());
  EXPECT_EQ(second_progress_value, progress_future.Take());

  const auto& second_applied_tasks = applied_tasks_future.Take();
  EXPECT_EQ(1u, second_applied_tasks.size());
  EXPECT_EQ(second_url.spec(), second_applied_tasks[0].url);
  EXPECT_EQ(second_task_description, second_applied_tasks[0].description);
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UiDelegateSetAppliedItemsAndSetComplete) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  const int progress = 50;
  base::RunLoop check_loop;
  base::RunLoop progress_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));

  // SetAppliedItems must be called as policy script returns valid result with
  // STARTED status
  EXPECT_CALL(ui_delegate(), SetAppliedItems(progress, _))
      .WillOnce([&progress_loop](long, const std::vector<PsstTask>&) {
        progress_loop.Quit();
      });
  // SetComplete must not be called as policy script returns STARTED status
  EXPECT_CALL(ui_delegate(), SetComplete()).Times(1);

  base::test::TestFuture<base::Value> user_script_insert_future;
  base::test::TestFuture<base::Value> policy_script_insert_future;

  // Create a user script return value
  auto script_params = base::Value(base::Value::Dict().Set("user", "value"));

  // prepare return value for policy script (status should be COMPLETED)
  auto policy_script_result =
      base::Value(base::Value::Dict()
                      .Set("status", "COMPLETED")
                      .Set("progress", progress)
                      .Set("applied_tasks",
                           base::Value::List().Append(
                               base::Value::Dict()
                                   .Set("url", "https://example1.com/task")
                                   .Set("description", "Ads Preferences"))));

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           script_params.Clone()));

  const auto policy_script_with_parameters = base::StrCat(
      {"const params = ",
       base::WriteJsonWithOptions(script_params.Clone(),
                                  base::JSONWriter::OPTIONS_PRETTY_PRINT)
           .value(),
       ";\n", policy_script});
  // Policy script executed, parameters added
  EXPECT_CALL(inject_script_callback(), Run(policy_script_with_parameters, _))
      .WillOnce(InsertScriptInPageCallback(&policy_script_insert_future,
                                           policy_script_result.Clone()));

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
  EXPECT_EQ(policy_script_result, policy_script_insert_future.Take());
  progress_loop.Run();
}

class PsstTabWebContentsObserverFeatureDisabledUnitTest
    : public PsstTabWebContentsObserverUnitTestBase {
 public:
  void SetUp() override {
    feature_list_.InitAndDisableFeature(psst::features::kEnablePsst);
    PsstTabWebContentsObserverUnitTestBase::SetUp();
  }
};

TEST_F(PsstTabWebContentsObserverFeatureDisabledUnitTest, DontCreate) {
  EXPECT_EQ(PsstTabWebContentsObserver::MaybeCreateForWebContents(
                web_contents(), browser_context(),
                std::make_unique<MockUiDelegate>(), prefs(), 2),
            nullptr);
}

}  // namespace psst

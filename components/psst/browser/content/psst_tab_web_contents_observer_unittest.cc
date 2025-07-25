// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
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

class MockPsstScriptsHandler
    : public PsstTabWebContentsObserver::ScriptsHandler {
 public:
  MockPsstScriptsHandler() = default;
  ~MockPsstScriptsHandler() override = default;

  MOCK_METHOD(void,
              InsertScriptInPage,
              (const std::string& script,
               PsstTabWebContentsObserver::InsertScriptInPageCallback cb),
              (override));
};

// testing::InvokeArgument<N> does not work with base::OnceCallback, so we
// define our own gMock action to run the 2nd argument.
ACTION_P(InsertScriptInPageCallback, loop, value) {
  std::move(
      const_cast<PsstTabWebContentsObserver::InsertScriptInPageCallback&>(arg1))
      .Run(value.Clone());
  loop->Quit();
}

class PsstTabWebContentsObserverUnitTestBase
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();

    psst::RegisterProfilePrefs(prefs_.registry());
    scripts_handler_ = new MockPsstScriptsHandler();
    rule_registry_ = std::make_unique<MockPsstRuleRegistry>();
    psst_web_contents_observer_ = base::WrapUnique<PsstTabWebContentsObserver>(
        new PsstTabWebContentsObserver(
            web_contents(), rule_registry_.get(), &prefs_,
            base::WrapUnique<MockPsstScriptsHandler>(scripts_handler_)));
  }

  void TearDown() override {
    content::RenderViewHostTestHarness::TearDown();
    scripts_handler_ = nullptr;
  }

  MockPsstRuleRegistry& psst_rule_registry() { return *rule_registry_.get(); }
  MockPsstScriptsHandler& scripts_handler() { return *scripts_handler_; }
  PrefService* prefs() { return &prefs_; }

  MatchedRule* CreateMatchedRule(const std::string& user_script,
                                 const std::string& policy_script) {
    return new MatchedRule("name", user_script, policy_script, 1);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  raw_ptr<MockPsstScriptsHandler> scripts_handler_;  // not owned
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
                web_contents(), browser_context(), prefs(), 2),
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
                web_contents.get(), &otr_browser_context, prefs(), 2),
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

  base::RunLoop first_nav_user_script_insert_loop;
  EXPECT_CALL(scripts_handler(), InsertScriptInPage(first_nav_user_script, _))
      .WillOnce(InsertScriptInPageCallback(&first_nav_user_script_insert_loop,
                                           base::Value()));

  DocumentOnLoadObserver first_nav_observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), first_navigation_url);
  first_nav_observer.Wait();

  first_nav_check_loop.Run();
  first_nav_user_script_insert_loop.Run();

  const std::string second_nav_user_script = "user2";
  const GURL second_navigation_url("https://example2.com");
  base::RunLoop second_nav_check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(second_navigation_url, _))
      .WillOnce(CheckIfMatchCallback(
          &second_nav_check_loop,
          CreateMatchedRule(second_nav_user_script, policy_script)));

  base::RunLoop second_nav_user_script_insert_loop;
  EXPECT_CALL(scripts_handler(), InsertScriptInPage(second_nav_user_script, _))
      .WillOnce(InsertScriptInPageCallback(&second_nav_user_script_insert_loop,
                                           base::Value()));
  EXPECT_CALL(scripts_handler(), InsertScriptInPage(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), second_navigation_url);
  observer.Wait();

  second_nav_check_loop.Run();
  second_nav_user_script_insert_loop.Run();
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

  base::RunLoop user_script_insert_loop;
  auto value = base::Value();
  EXPECT_CALL(scripts_handler(), InsertScriptInPage(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_loop,
                                           std::move(value)));
  EXPECT_CALL(scripts_handler(), InsertScriptInPage(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  auto simulator =
      content::NavigationSimulator::CreateBrowserInitiated(url, web_contents());
  simulator->Redirect(redirect_target);
  simulator->Commit();

  observer.Wait();
  check_loop.Run();
  user_script_insert_loop.Run();
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
  EXPECT_CALL(scripts_handler(), InsertScriptInPage).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsEmptyHasPolicyScript) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::RunLoop user_script_insert_loop;
  auto value = base::Value();

  EXPECT_CALL(scripts_handler(), InsertScriptInPage(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_loop,
                                           std::move(value)));
  EXPECT_CALL(scripts_handler(), InsertScriptInPage(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();
  check_loop.Run();
  user_script_insert_loop.Run();
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsEmptyNoPolicyScript) {
  const std::string user_script = "user";
  const std::string policy_script = "";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::RunLoop user_script_insert_loop;
  auto value = base::Value();

  EXPECT_CALL(scripts_handler(), InsertScriptInPage(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_loop,
                                           std::move(value)));
  EXPECT_CALL(scripts_handler(), InsertScriptInPage(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  user_script_insert_loop.Run();
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsDictHasPolicyScript) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::RunLoop user_script_insert_loop;
  base::RunLoop policy_script_insert_loop;
  auto dict = base::Value(base::Value::Dict());
  auto value = base::Value();

  EXPECT_CALL(scripts_handler(), InsertScriptInPage(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_loop,
                                           std::move(dict)));
  EXPECT_CALL(scripts_handler(), InsertScriptInPage(policy_script, _))
      .WillOnce(InsertScriptInPageCallback(&policy_script_insert_loop,
                                           std::move(value)));

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  user_script_insert_loop.Run();
  policy_script_insert_loop.Run();
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsDictNoPolicyScript) {
  const std::string user_script = "user";
  const std::string policy_script = "";
  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::RunLoop user_script_insert_loop;
  auto dict = base::Value(base::Value::Dict());
  auto value = base::Value();

  EXPECT_CALL(scripts_handler(), InsertScriptInPage(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_loop,
                                           std::move(dict)));
  EXPECT_CALL(scripts_handler(), InsertScriptInPage(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  user_script_insert_loop.Run();
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
                web_contents(), browser_context(), prefs(), 2),
            nullptr);
}

}  // namespace psst

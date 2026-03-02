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
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/psst/browser/core/psst_rule_registry.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "brave/components/psst/common/psst_metadata_schema.h"
#include "brave/components/psst/common/psst_script_responses.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_entry_restore_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/replaced_navigation_entry_data.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/page_state/page_state.h"
#include "url/origin.h"

using ::testing::_;
using ::testing::InvokeArgument;

namespace content {

class MockNavigationEntry : public NavigationEntry {
 public:
  MockNavigationEntry() = default;
  ~MockNavigationEntry() override = default;

  MOCK_METHOD(bool, IsInitialEntry, (), (const, override));
  MOCK_METHOD(int, GetUniqueID, (), (const, override));
  MOCK_METHOD(content::PageType, GetPageType, (), (const, override));
  MOCK_METHOD(void, SetURL, (const GURL& url), (override));
  MOCK_METHOD(const GURL&, GetURL, (), (const, override));
  MOCK_METHOD(void, SetBaseURLForDataURL, (const GURL& url), (override));
  MOCK_METHOD(const GURL&, GetBaseURLForDataURL, (), (const, override));

  MOCK_METHOD(void,
              SetReferrer,
              (const content::Referrer& referrer),
              (override));
  MOCK_METHOD(const content::Referrer&, GetReferrer, (), (const, override));
  MOCK_METHOD(void, SetVirtualURL, (const GURL& url), (override));
  MOCK_METHOD(const GURL&, GetVirtualURL, (), (const, override));
  MOCK_METHOD(void, SetTitle, (std::u16string title), (override));
  MOCK_METHOD(const std::u16string&, GetTitle, (), (const, override));
  MOCK_METHOD(void,
              SetApplicationTitle,
              (const std::u16string& application_title),
              (override));
  MOCK_METHOD(const std::optional<std::u16string>&,
              GetApplicationTitle,
              (),
              (const, override));
  MOCK_METHOD(void,
              SetPageState,
              (const blink::PageState& state,
               NavigationEntryRestoreContext* context),
              (override));
  MOCK_METHOD(blink::PageState, GetPageState, (), (const, override));
  MOCK_METHOD(const std::u16string&, GetTitleForDisplay, (), (const, override));
  MOCK_METHOD(bool, IsViewSourceMode, (), (const, override));
  MOCK_METHOD(void,
              SetTransitionType,
              (ui::PageTransition transition_type),
              (override));
  MOCK_METHOD(ui::PageTransition, GetTransitionType, (), (const, override));
  MOCK_METHOD(const GURL&, GetUserTypedURL, (), (const, override));
  MOCK_METHOD(void, SetHasPostData, (bool has_post_data), (override));
  MOCK_METHOD(bool, GetHasPostData, (), (const, override));
  MOCK_METHOD(void, SetPostID, (int64_t post_id), (override));
  MOCK_METHOD(int64_t, GetPostID, (), (const, override));
  MOCK_METHOD(void,
              SetPostData,
              (const scoped_refptr<network::ResourceRequestBody>& data),
              (override));
  MOCK_METHOD(const scoped_refptr<const network::ResourceRequestBody>,
              GetPostData,
              (),
              (const, override));
  MOCK_METHOD(FaviconStatus&, GetFavicon, (), (override));
  MOCK_METHOD(SSLStatus&, GetSSL, (), (override));
  MOCK_METHOD(void,
              SetOriginalRequestURL,
              (const GURL& original_url),
              (override));
  MOCK_METHOD(const GURL&, GetOriginalRequestURL, (), (const, override));
  MOCK_METHOD(void, SetIsOverridingUserAgent, (bool override_ua), (override));
  MOCK_METHOD(bool, GetIsOverridingUserAgent, (), (const, override));
  MOCK_METHOD(void, SetTimestamp, (base::Time timestamp), (override));
  MOCK_METHOD(base::Time, GetTimestamp, (), (const, override));
  MOCK_METHOD(void, SetCanLoadLocalResources, (bool allow), (override));
  MOCK_METHOD(bool, GetCanLoadLocalResources, (), (const, override));
  MOCK_METHOD(void, SetHttpStatusCode, (int http_status_code), (override));
  MOCK_METHOD(int, GetHttpStatusCode, (), (const, override));
  MOCK_METHOD(void,
              SetRedirectChain,
              (const std::vector<GURL>& redirects),
              (override));
  MOCK_METHOD(const std::vector<GURL>&,
              GetRedirectChain,
              (),
              (const, override));
  MOCK_METHOD(const std::optional<ReplacedNavigationEntryData>&,
              GetReplacedEntryData,
              (),
              (const, override));
  MOCK_METHOD(bool, IsRestored, (), (const, override));
  MOCK_METHOD(std::string, GetExtraHeaders, (), (const, override));
  MOCK_METHOD(void,
              AddExtraHeaders,
              (const std::string& extra_headers),
              (override));
  MOCK_METHOD(int64_t,
              GetMainFrameDocumentSequenceNumber,
              (),
              (const, override));
  MOCK_METHOD(bool,
              IsPossiblySkippableAdEntryForTesting,
              (),
              (const, override));
};

}  // namespace content

namespace psst {

namespace {
constexpr base::TimeDelta kScriptTimeout = base::Seconds(15);

const char signed_user_id_prop_name[] = "user_id";
const char tasks_prop_name[] = "tasks";
const char site_name_prop_name[] = "name";

MATCHER_P4(PsstWebsiteSettingsEq,
           consent_status,
           script_version,
           user_id,
           urls_to_skip,
           "PsstWebsiteSettings with consent_status=" +
               ::testing::PrintToString(consent_status) +
               ", script_version=" + ::testing::PrintToString(script_version) +
               ", user_id=" + ::testing::PrintToString(user_id) +
               ", urls_to_skip=" + ::testing::PrintToString(urls_to_skip)) {
  return arg.consent_status == consent_status &&
         arg.script_version == script_version && arg.user_id == user_id &&
         arg.urls_to_skip == urls_to_skip;
}

std::string IgnorePsstSettingsField(const std::string& arg) {
  std::string modified_arg = arg;
  if (size_t psst_settings_start =
          modified_arg.find("\"psst_settings_status\":");
      psst_settings_start != std::string::npos) {
    size_t comma_pos = modified_arg.find(",", psst_settings_start);
    if (comma_pos != std::string::npos) {
      modified_arg.erase(psst_settings_start,
                         comma_pos - psst_settings_start + 1);
    }
  }
  return modified_arg;
}

MATCHER_P(ScriptWithParametersEq,
          expected_policy_script,
          "expected_policy_script=" +
              ::testing::PrintToString(expected_policy_script)) {
  // Remove psst_settings_status field from the script string
  // as it contain navigation entry id which can be different
  return IgnorePsstSettingsField(arg) ==
         IgnorePsstSettingsField(expected_policy_script);
}

}  // namespace

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
ACTION_P(InsertPolicyScriptInPageCallback, future, value) {
  std::move(
      const_cast<PsstTabWebContentsObserver::InsertScriptInPageCallback&>(arg1))
      .Run(value.Clone());
  future->SetValue(value.Clone());
}
ACTION_P(ShowCallback, future, disabled_checks) {
  std::move(
      const_cast<PsstTabWebContentsObserver::PsstUiDelegate::ConsentCallback&>(
          arg4))
      .Run(disabled_checks);
  future->SetValue(disabled_checks);
}
ACTION_P(InsertScriptInPageDelayedCallback,
         future,
         task_environment,
         delay_in_secs,
         value) {
  task_environment->GetMainThreadTaskRunner()->PostDelayedTask(
      FROM_HERE,
      base::BindLambdaForTesting(
          [callback = std::move(
               const_cast<
                   PsstTabWebContentsObserver::InsertScriptInPageCallback&>(
                   arg1)),
           fut = future, val = value.Clone()]() {
            std::move(
                const_cast<
                    PsstTabWebContentsObserver::InsertScriptInPageCallback&>(
                    callback))
                .Run(val.Clone());
            fut->SetValue(val.Clone());
          }),
      base::Seconds(delay_in_secs));
  task_environment->FastForwardBy(base::Seconds(delay_in_secs));
}

class MockUiDelegate : public PsstTabWebContentsObserver::PsstUiDelegate {
 public:
  MockUiDelegate() = default;
  ~MockUiDelegate() override = default;

  MOCK_METHOD(void,
              Show,
              (const url::Origin& origin,
               PsstWebsiteSettings dialog_data,
               const std::string& site_name,
               base::ListValue tasks,
               PsstUiDelegate::ConsentCallback apply_changes_callback),
              (override));

  MOCK_METHOD(void,
              UpdateTasks,
              (long progress,
               const std::vector<PolicyTask>& applied_tasks,
               const mojom::PsstStatus status),
              (override));

  MOCK_METHOD(std::optional<PsstWebsiteSettings>,
              GetPsstWebsiteSettings,
              (const url::Origin& origin, const std::string& user_id),
              (override));
};

class PsstTabWebContentsObserverUnitTestBase
    : public content::RenderViewHostTestHarness {
 public:
  PsstTabWebContentsObserverUnitTestBase()
      : content::RenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~PsstTabWebContentsObserverUnitTestBase() override = default;

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

  PsstTabWebContentsObserver& psst_web_contents_observer() {
    return *psst_web_contents_observer_;
  }

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

  auto restored_entry = std::make_unique<content::MockNavigationEntry>();

  EXPECT_CALL(*restored_entry.get(), GetURL())
      .WillOnce(testing::ReturnRef(url));
  EXPECT_CALL(*restored_entry.get(), IsRestored())
      .WillOnce(testing::Return(true));

  content::LoadCommittedDetails details;
  details.entry = restored_entry.get();
  psst_web_contents_observer().NavigationEntryCommitted(details);
  EXPECT_EQ(nullptr, restored_entry->GetUserData("should_process_key"));
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

  // Called twice, once for each navigation, with Failed status as user script
  // result is empty in both cases
  EXPECT_CALL(ui_delegate(), UpdateTasks(100, _, mojom::PsstStatus::kFailed))
      .Times(2);

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

  // Called once, with Failed status as user script result is empty
  EXPECT_CALL(ui_delegate(), UpdateTasks(100, _, mojom::PsstStatus::kFailed))
      .Times(1);

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

  EXPECT_CALL(ui_delegate(), UpdateTasks(100, _, mojom::PsstStatus::kFailed))
      .Times(1);

  // User script result is an empty value
  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           base::Value()));
  // No policy script executed
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();
  check_loop.Run();
  EXPECT_EQ(base::Value(), user_script_insert_future.Take());
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

  // Called once, with Failed status as user script result is empty dictionary
  EXPECT_CALL(ui_delegate(), UpdateTasks(100, _, mojom::PsstStatus::kFailed))
      .Times(1);

  base::test::TestFuture<base::Value> user_script_insert_future;

  // User script result is an empty dictionary
  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           base::Value(base::DictValue())));
  // No policy script executed
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(base::DictValue(), user_script_insert_future.Take());
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

  // Called once, with Failed status as user script result is empty dictionary
  EXPECT_CALL(ui_delegate(), UpdateTasks(100, _, mojom::PsstStatus::kFailed))
      .Times(1);

  // User script result is an empty dictionary
  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           base::Value(base::DictValue())));
  // No policy script executed
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();

  check_loop.Run();
  EXPECT_EQ(base::DictValue(), user_script_insert_future.Take());
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UserScriptReturnsUserHasPolicyScript) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const std::string user_id = "unique_user_id";

  const GURL url("https://example1.com");
  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));
  base::test::TestFuture<base::Value> user_script_insert_future;
  base::test::TestFuture<const std::vector<std::string>&>
      user_accept_psst_settings_future;
  base::test::TestFuture<base::Value> policy_script_insert_future;

  // User script result is an dictionary, and user key is not empty
  auto script_params = base::Value(
      base::DictValue()
          .Set(signed_user_id_prop_name, user_id)
          .Set(tasks_prop_name,
               base::ListValue().Append(base::DictValue()
                                            .Set("url", "https://example1.com")
                                            .Set("description", "settings")))
          .Set(site_name_prop_name, "example"));

  // Policy script result is a dictionary, but it is not deserializable
  auto policy_script_result =
      base::Value(base::DictValue().Set("prop", "value"));

  // Call UI delegate method once (Failed state) as policy_script_result
  // is not deserializable
  EXPECT_CALL(ui_delegate(), UpdateTasks(100, _, mojom::PsstStatus::kFailed))
      .Times(0);

  EXPECT_CALL(ui_delegate(),
              GetPsstWebsiteSettings(url::Origin::Create(url), user_id));

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           script_params.Clone()));

  EXPECT_CALL(ui_delegate(),
              Show(url::Origin::Create(url),
                   PsstWebsiteSettingsEq(ConsentStatus::kAsk, 1, user_id,
                                         std::vector<std::string>()),
                   "example", _, _))
      .WillOnce(ShowCallback(&user_accept_psst_settings_future,
                             std::vector<std::string>()));

  auto policy_script_params = script_params.Clone();
  // We expect the next two parameters to be added by the
  // PrepareParametersForPolicyExecution
  policy_script_params.GetDict().Set("initial_execution", true);
  policy_script_params.GetDict().Set("psst_settings_status", 0);  // any value

  const auto script_with_parameters = base::StrCat(
      {"const params = ",
       base::WriteJsonWithOptions(policy_script_params,
                                  base::JSONWriter::OPTIONS_PRETTY_PRINT)
           .value(),
       ";\n", policy_script});

  // Policy script executed, parameters added
  EXPECT_CALL(inject_script_callback(),
              Run(ScriptWithParametersEq(script_with_parameters), _))
      .WillOnce(InsertPolicyScriptInPageCallback(&policy_script_insert_future,
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

  // Call UI delegate method once (Failed state) as user_script_result
  // has empty user value
  EXPECT_CALL(ui_delegate(), UpdateTasks(100, _, mojom::PsstStatus::kFailed))
      .Times(1);

  // User script result is an dictionary, but user key is empty
  auto script_params = base::Value(base::DictValue().Set("user", ""));

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
  const std::string user_id = "unique_user_id";

  base::RunLoop check_loop;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));

  base::test::TestFuture<base::Value> user_script_insert_future;
  base::test::TestFuture<const std::vector<std::string>&>
      user_accept_psst_settings_future;
  base::test::TestFuture<base::Value> policy_script_insert_future;

  EXPECT_CALL(ui_delegate(),
              GetPsstWebsiteSettings(url::Origin::Create(url), user_id));

  // Create a dictionary with unsupported blob storage value
  auto script_params = base::Value(
      base::DictValue()
          .Set(signed_user_id_prop_name, user_id)
          .Set(tasks_prop_name,
               base::ListValue().Append(base::DictValue()
                                            .Set("url", "https://example1.com")
                                            .Set("description", "settings")))
          .Set(site_name_prop_name, "example")
          .Set("prop",
               base::Value(base::Value::BlobStorage{0x01, 0x02, 0x03})));
  auto policy_script_result = base::Value();

  // Call UI delegate method once (Failed state) as policy_script_result is
  // empty
  EXPECT_CALL(ui_delegate(), UpdateTasks(100, _, mojom::PsstStatus::kFailed))
      .Times(1);

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           script_params.Clone()));

  EXPECT_CALL(ui_delegate(),
              Show(url::Origin::Create(url),
                   PsstWebsiteSettingsEq(ConsentStatus::kAsk, 1, user_id,
                                         std::vector<std::string>()),
                   "example", _, _))
      .WillOnce(ShowCallback(&user_accept_psst_settings_future,
                             std::vector<std::string>()));

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

TEST_F(PsstTabWebContentsObserverUnitTest, UiDelegateUpdateTasksCalled) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  const std::string task_description = "task description";
  const std::string user_id = "unique_user_id";
  const int progress = 50;
  base::RunLoop check_loop;
  base::test::TestFuture<long> progress_future;
  base::test::TestFuture<std::vector<PolicyTask>> applied_tasks_future;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));

  // UpdateTasks must be called with correct parameters, as policy script
  // returns valid result
  EXPECT_CALL(ui_delegate(),
              UpdateTasks(progress, _, mojom::PsstStatus::kInProgress))
      .WillOnce([&progress_future, &applied_tasks_future](
                    long progress_value,
                    const std::vector<PolicyTask>& applied_tasks,
                    const mojom::PsstStatus status) {
        std::vector<PolicyTask> tasks;
        std::ranges::for_each(applied_tasks, [&tasks](const PolicyTask& task) {
          tasks.push_back(task.Clone());
        });
        applied_tasks_future.SetValue(std::move(tasks));
        progress_future.SetValue(progress_value);
      });

  base::test::TestFuture<base::Value> user_script_insert_future;
  base::test::TestFuture<const std::vector<std::string>&>
      user_accept_psst_settings_future;
  base::test::TestFuture<base::Value> policy_script_insert_future;

  EXPECT_CALL(ui_delegate(),
              GetPsstWebsiteSettings(url::Origin::Create(url), user_id));

  // Create a user script return value
  auto script_params = base::Value(
      base::DictValue()
          .Set(signed_user_id_prop_name, user_id)
          .Set(tasks_prop_name,
               base::ListValue().Append(base::DictValue()
                                            .Set("url", "https://example1.com")
                                            .Set("description", "settings")))
          .Set(site_name_prop_name, "example"));

  // prepare return value for policy script (status should be STARTED)
  auto policy_script_result = base::Value(
      base::DictValue()
          .Set("result", false)
          .Set("next_url", "https://example1.com/next")
          .Set("psst",
               base::DictValue()
                   .Set("progress", progress)
                   .Set("applied_tasks",
                        base::ListValue().Append(
                            base::DictValue()
                                .Set("url", url.spec())
                                .Set("description", task_description)))));

  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageCallback(&user_script_insert_future,
                                           script_params.Clone()));

  EXPECT_CALL(ui_delegate(),
              Show(url::Origin::Create(url),
                   PsstWebsiteSettingsEq(ConsentStatus::kAsk, 1, user_id,
                                         std::vector<std::string>()),
                   "example", _, _))
      .WillOnce(ShowCallback(&user_accept_psst_settings_future,
                             std::vector<std::string>()));

  auto policy_script_params = script_params.Clone();
  // We expect the next two parameters to be added by the
  // PrepareParametersForPolicyExecution
  policy_script_params.GetDict().Set("initial_execution", true);
  policy_script_params.GetDict().Set("psst_settings_status", 0);  // any value

  const auto policy_script_with_parameters = base::StrCat(
      {"const params = ",
       base::WriteJsonWithOptions(policy_script_params,
                                  base::JSONWriter::OPTIONS_PRETTY_PRINT)
           .value(),
       ";\n", policy_script});
  // Policy script executed, parameters added
  EXPECT_CALL(inject_script_callback(),
              Run(ScriptWithParametersEq(policy_script_with_parameters), _))
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

  const auto& applied_tasks = applied_tasks_future.Take();
  EXPECT_EQ(1u, applied_tasks.size());
  EXPECT_EQ(url.spec(), applied_tasks[0].url);
  EXPECT_EQ(task_description, applied_tasks[0].description);
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       UiDelegateUpdateTasksCalledAsUserScriptTimeout) {
  const std::string user_script = "user";
  const std::string policy_script = "policy";
  const GURL url("https://example1.com");
  const std::string task_description = "task description";
  const int progress = 100;
  base::RunLoop check_loop;
  base::test::TestFuture<long> progress_future;
  base::test::TestFuture<std::vector<PolicyTask>> applied_tasks_future;
  EXPECT_CALL(psst_rule_registry(), CheckIfMatch(url, _))
      .WillOnce(CheckIfMatchCallback(
          &check_loop, CreateMatchedRule(user_script, policy_script)));

  // UpdateTasks must be called with in case of timeout failure, to indicate
  // that the complete workflow has ended with error.
  EXPECT_CALL(ui_delegate(),
              UpdateTasks(progress, _, mojom::PsstStatus::kFailed))
      .WillOnce([&progress_future, &applied_tasks_future](
                    long progress_value,
                    const std::vector<PolicyTask>& applied_tasks,
                    const mojom::PsstStatus status) {
        std::vector<PolicyTask> tasks;
        std::ranges::for_each(applied_tasks, [&tasks](const PolicyTask& task) {
          tasks.push_back(task.Clone());
        });
        applied_tasks_future.SetValue(std::move(tasks));
        progress_future.SetValue(progress_value);
      });

  base::test::TestFuture<base::Value> user_script_insert_future;

  // Create a user script return value
  auto script_params = base::Value(base::DictValue().Set("user", "value"));

  // User script's callback is delayed, causing the flow to fail
  EXPECT_CALL(inject_script_callback(), Run(user_script, _))
      .WillOnce(InsertScriptInPageDelayedCallback(
          &user_script_insert_future, task_environment(),
          kScriptTimeout.InSeconds() + 1, script_params.Clone()));

  // No policy script executed
  EXPECT_CALL(inject_script_callback(), Run(policy_script, _)).Times(0);

  DocumentOnLoadObserver observer(web_contents());
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
  observer.Wait();
  check_loop.Run();
  EXPECT_EQ(script_params, user_script_insert_future.Take());
  EXPECT_EQ(progress, progress_future.Take());

  EXPECT_TRUE(applied_tasks_future.Take().empty());
  // TODO(https://github.com/brave/brave-browser/issues/49317) We need to check
  // that script result callbacks are not in queue
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

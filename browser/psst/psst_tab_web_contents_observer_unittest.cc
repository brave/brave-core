// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/components/psst/browser/content/psst_scripts_handler.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {
std::unique_ptr<content::BrowserContext> BuildBrowserContext() {
  TestingProfile::Builder builder;
  auto prefs = std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
  ::RegisterUserProfilePrefs(prefs->registry());
  builder.SetPrefService(std::move(prefs));
  auto original_profile = builder.Build();
  return original_profile;
}
}  // namespace

class MockPsstScriptsHandler : public PsstScriptsHandler {
 public:
  MockPsstScriptsHandler() = default;
  ~MockPsstScriptsHandler() override = default;

  MOCK_METHOD(void, Start, (), (override));
};

class PsstTabWebContentsObserverUnitTest
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(psst::features::kEnablePsst);
    content::RenderViewHostTestHarness::SetUp();

    auto script_handler = std::make_unique<MockPsstScriptsHandler>();
    script_handler_ = script_handler.get();
    observer_ = PsstTabWebContentsObserver::MaybeCreateForWebContents(
        web_contents(), profile(), profile()->GetPrefs(),
        ISOLATED_WORLD_ID_BRAVE_INTERNAL);
    observer_->script_handler_ = std::move(script_handler);
    page_checker_ = observer_->page_checker_.get();
  }
  PsstTabWebContentsObserver* GetObserver() { return observer_.get(); }

  Profile* profile() {
    return static_cast<Profile*>(web_contents()->GetBrowserContext());
  }

  PsstShouldProcessPageChecker* GetPageChecker() { return page_checker_; }
  MockPsstScriptsHandler* GetScriptHandler() { return script_handler_; }

  void TearDown() override {
    page_checker_ = nullptr;
    script_handler_ = nullptr;
    observer_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    return BuildBrowserContext();
  }

 private:
  std::unique_ptr<PsstTabWebContentsObserver> observer_;
  raw_ptr<PsstShouldProcessPageChecker> page_checker_;
  raw_ptr<MockPsstScriptsHandler> script_handler_;

  base::test::ScopedFeatureList feature_list_;
};

class PsstTabWebContentsObserverFeatureEnabledUnitTest
    : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(psst::features::kEnablePsst);
    content::RenderViewHostTestHarness::SetUp();
  }

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    return BuildBrowserContext();
  }

  content::WebContents* web_contents() {
    return content::RenderViewHostTestHarness::web_contents();
  }

  Profile* profile() {
    return static_cast<Profile*>(web_contents()->GetBrowserContext());
  }

  Profile* otr_profile() {
    TestingProfile::Builder profile_builder;
    return profile_builder.BuildIncognito(
        static_cast<TestingProfile*>(profile()));
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

class PsstTabWebContentsObserverFeatureDisabledUnitTest
    : public PsstTabWebContentsObserverFeatureEnabledUnitTest {
 public:
  void SetUp() override {
    feature_list_.InitAndDisableFeature(psst::features::kEnablePsst);
    content::RenderViewHostTestHarness::SetUp();
  }
};

TEST_F(PsstTabWebContentsObserverFeatureEnabledUnitTest, CreateObserver) {
  EXPECT_TRUE(PsstTabWebContentsObserver::MaybeCreateForWebContents(
      web_contents(), profile(), profile()->GetPrefs(),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL));
  EXPECT_FALSE(PsstTabWebContentsObserver::MaybeCreateForWebContents(
      web_contents(), otr_profile(), otr_profile()->GetPrefs(),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

TEST_F(PsstTabWebContentsObserverFeatureDisabledUnitTest, CreateObserver) {
  EXPECT_FALSE(PsstTabWebContentsObserver::MaybeCreateForWebContents(
      web_contents(), profile(), profile()->GetPrefs(),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL));
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldNotProcessRestoredNavigationEntry) {
  content::NavigationController& controller = web_contents()->GetController();
  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);

  std::unique_ptr<content::NavigationEntry> restored_entry =
      content::NavigationEntry::Create();
  restored_entry->SetURL(GURL("https://example1.com"));
  restored_entry->SetTitle(u"Restored Page");

  std::vector<std::unique_ptr<NavigationEntry>> entries;
  entries.push_back(std::move(restored_entry));

  controller.Restore(0 /* selected_index */, content::RestoreType::kRestored,
                     &entries);

  controller.LoadIfNecessary();

  auto navigation_simulator =
      content::NavigationSimulator::CreateFromPending(controller);
  navigation_simulator->Commit();
  EXPECT_FALSE(GetObserver()->should_process_);
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldNotProcessIfNotPrimaryMainFrame) {
  const GURL native_url("https://example1.com");

  EXPECT_CALL(*GetScriptHandler(), Start).Times(1);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             native_url);
  auto* main_rfh = web_contents()->GetPrimaryMainFrame();
  auto* child_rfh =
      content::RenderFrameHostTester::For(main_rfh)->AppendChild("subframe");
  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);

  auto sim = content::NavigationSimulator::CreateRendererInitiated(
      GURL("https://sub.example.com"), child_rfh);
  sim->Commit();
  EXPECT_FALSE(GetObserver()->should_process_);
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldNotProcessIfNavigationNotCommitted) {
  auto simulator = content::NavigationSimulator::CreateBrowserInitiated(
      GURL("https://example.com"), web_contents());

  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);
  // Simulate navigation start but NOT commit
  simulator->Start();
  simulator->Fail(net::ERR_ABORTED);  // Simulates cancel before commit
  EXPECT_FALSE(GetObserver()->should_process_);
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldNotProcessIfSameDocumentNavigation) {
  const GURL native_url("https://example1.com");

  EXPECT_CALL(*GetScriptHandler(), Start).Times(1);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             native_url);

  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);
  auto sim = content::NavigationSimulator::CreateRendererInitiated(
      GURL(base::JoinString({native_url.spec(), "anchor"}, "#")),
      web_contents()->GetPrimaryMainFrame());
  sim->CommitSameDocument();
  EXPECT_FALSE(GetObserver()->should_process_);
}

TEST_F(PsstTabWebContentsObserverUnitTest, StartScriptHandlerIfEnabled) {
  profile()->GetPrefs()->SetBoolean(prefs::kPsstEnabled, true);
  EXPECT_CALL(*GetScriptHandler(), Start).Times(1);
  const GURL native_url("https://example1.com");
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             native_url);
  EXPECT_FALSE(GetObserver()->should_process_);

  profile()->GetPrefs()->SetBoolean(prefs::kPsstEnabled, false);
  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             native_url);
  EXPECT_TRUE(GetObserver()->should_process_);
}

}  // namespace psst

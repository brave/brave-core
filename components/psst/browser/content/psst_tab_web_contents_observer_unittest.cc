// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "brave/test/base/components_unit_test.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

class MockPsstScriptsHandler
    : public PsstTabWebContentsObserver::ScriptsHandler {
 public:
  MockPsstScriptsHandler() = default;
  ~MockPsstScriptsHandler() override = default;

  MOCK_METHOD(void, Start, (), (override));
};

class PsstTabWebContentsObserverUnitTestBase
    : public content::RenderViewHostTestHarness,
      public ComponentsUnitTest {
 public:
  void SetUp() override {
    ComponentsUnitTest::SetUp();
    content::RenderViewHostTestHarness::SetUp();

    psst::RegisterProfilePrefs(prefs_.registry());
    observer_ = base::WrapUnique<PsstTabWebContentsObserver>(
        new PsstTabWebContentsObserver(
            web_contents(), &prefs_,
            std::make_unique<MockPsstScriptsHandler>()));
  }
  PsstTabWebContentsObserver* GetObserver() { return observer_.get(); }
  MockPsstScriptsHandler* GetScriptHandler() {
    return static_cast<MockPsstScriptsHandler*>(
        observer_->script_handler_.get());
  }
  PrefService* prefs() { return &prefs_; }

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  std::unique_ptr<PsstTabWebContentsObserver> observer_;
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
  content::NavigationController& controller = web_contents()->GetController();
  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);

  std::unique_ptr<content::NavigationEntry> restored_entry =
      content::NavigationEntry::Create();
  auto url = GURL("https://example1.com");
  restored_entry->SetURL(url);
  restored_entry->SetTitle(u"Restored Page");

  std::vector<std::unique_ptr<content::NavigationEntry>> entries;
  entries.push_back(std::move(restored_entry));

  controller.Restore(0 /* selected_index */, content::RestoreType::kRestored,
                     &entries);

  controller.LoadIfNecessary();

  auto navigation_simulator =
      content::NavigationSimulator::CreateFromPending(controller);
  navigation_simulator->Commit();
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
}

TEST_F(PsstTabWebContentsObserverUnitTest,
       ShouldNotProcessIfNavigationNotCommitted) {
  auto simulator = content::NavigationSimulator::CreateBrowserInitiated(
      GURL("https://example.com"), web_contents());

  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);
  // Simulate navigation start but NOT commit
  simulator->Start();
  simulator->Fail(net::ERR_ABORTED);  // Simulates cancel before commit
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
}

TEST_F(PsstTabWebContentsObserverUnitTest, PrefEnabledStartScriptHandler) {
  const GURL url("https://example1.com");
  prefs()->SetBoolean(prefs::kPsstEnabled, true);
  EXPECT_CALL(*GetScriptHandler(), Start).Times(1);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
}

TEST_F(PsstTabWebContentsObserverUnitTest, PrefDisabledDontStartScriptHandler) {
  const GURL url("https://example1.com");
  prefs()->SetBoolean(prefs::kPsstEnabled, false);
  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             url);
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

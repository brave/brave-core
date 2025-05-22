// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/psst/browser/content/psst_scripts_handler.h"
#include "brave/components/psst/common/features.h"
#include "brave/components/psst/common/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

class MockPsstPageChecker : public PsstShouldProcessPageChecker {
 public:
  MockPsstPageChecker() = default;
  ~MockPsstPageChecker() override = default;

  MOCK_METHOD(bool,
              ShouldProcess,
              (content::NavigationEntry * entry),
              (const, override));
};

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

    auto page_checker = std::make_unique<MockPsstPageChecker>();
    page_checker_ = page_checker.get();

    auto script_handler = std::make_unique<MockPsstScriptsHandler>();
    script_handler_ = script_handler.get();
    observer_ = base::WrapUnique<PsstTabWebContentsObserver>(
        new PsstTabWebContentsObserver(web_contents(), profile()->GetPrefs(),
                                       std::move(page_checker),
                                       std::move(script_handler)));
  }
  PsstTabWebContentsObserver* GetObserver() { return observer_.get(); }

  Profile* profile() {
    return static_cast<Profile*>(web_contents()->GetBrowserContext());
  }

  MockPsstPageChecker* GetPageChecker() { return page_checker_; }
  MockPsstScriptsHandler* GetScriptHandler() { return script_handler_; }

  void TearDown() override {
    page_checker_ = nullptr;
    script_handler_ = nullptr;
    observer_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    ::RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    return builder.Build();
  }

 private:
  std::unique_ptr<PsstTabWebContentsObserver> observer_;
  raw_ptr<MockPsstPageChecker> page_checker_;
  raw_ptr<MockPsstScriptsHandler> script_handler_;

  base::test::ScopedFeatureList feature_list_;
};

TEST_F(PsstTabWebContentsObserverUnitTest, PrimaryPageChanged) {
  const GURL native_url("https://example1.com");
  SetPsstEnabledState(profile()->GetPrefs(), false);

  EXPECT_CALL(*GetPageChecker(), ShouldProcess)
      .WillOnce(testing::Return(false));
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             native_url);
  EXPECT_FALSE(GetObserver()->should_process_);

  EXPECT_CALL(*GetPageChecker(), ShouldProcess).WillOnce(testing::Return(true));
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             native_url);
  EXPECT_TRUE(GetObserver()->should_process_);
}

TEST_F(PsstTabWebContentsObserverUnitTest, DocumentOnLoadCompletedScriptStart) {
  SetPsstEnabledState(profile()->GetPrefs(), true);
  EXPECT_CALL(*GetPageChecker(), ShouldProcess).WillOnce(testing::Return(true));
  EXPECT_CALL(*GetScriptHandler(), Start).Times(1);
  const GURL native_url("https://example1.com");
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             native_url);
  EXPECT_FALSE(GetObserver()->should_process_);

  SetPsstEnabledState(profile()->GetPrefs(), false);
  EXPECT_CALL(*GetPageChecker(), ShouldProcess).WillOnce(testing::Return(true));
  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             native_url);
  EXPECT_TRUE(GetObserver()->should_process_);

  SetPsstEnabledState(profile()->GetPrefs(), true);
  EXPECT_CALL(*GetPageChecker(), ShouldProcess)
      .WillOnce(testing::Return(false));
  EXPECT_CALL(*GetScriptHandler(), Start).Times(0);
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             native_url);
  EXPECT_FALSE(GetObserver()->should_process_);
}

}  // namespace psst

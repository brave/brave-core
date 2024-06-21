/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/simple_test_clock.h"
#include "brave/browser/brave_local_state_prefs.h"
#include "brave/browser/web_discovery/web_discovery_cta_util.h"
#include "brave/browser/web_discovery/web_discovery_tab_helper.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_test_util.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/search_engines/template_url_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

class WebDiscoveryCTATest : public testing::Test {
 public:
  WebDiscoveryCTATest() = default;

  void SetUp() override {
    // Setup g_browser_process because local_state() is refered during the
    // TemplateURLServiceTestUtil initialization.
    RegisterLocalState(test_local_state_.registry());
    TestingBrowserProcess::GetGlobal()->SetLocalState(&test_local_state_);
    test_clock_.SetNow(base::Time::Now());
    test_util_ = std::make_unique<TemplateURLServiceTestUtil>();
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
    ASSERT_TRUE(web_contents_.get());
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
  }

  WebDiscoveryTabHelper* tab_helper() {
    return WebDiscoveryTabHelper::FromWebContents(web_contents());
  }

  void SetBraveSearchAsDefaultProvider() {
    // Set brave search as a default provider.
    std::unique_ptr<TemplateURL> brave = CreateTestTemplateURL(
        u"brave", "https://search.brave.com/", std::string(),
        base::Time::FromTimeT(100), false,
        TemplateURLData::CreatedByPolicy::kNoPolicy,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE);
    service()->SetUserSelectedDefaultSearchProvider(brave.get());
    ASSERT_TRUE(IsBraveSearchDefault());
  }

  void SetNonBraveSearchAsDefaultProvider() {
    // Set brave search as a default provider.
    std::unique_ptr<TemplateURL> google = CreateTestTemplateURL(
        u"google", "https://www.google.com/", std::string(),
        base::Time::FromTimeT(100), false,
        TemplateURLData::CreatedByPolicy::kNoPolicy,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE);
    service()->SetUserSelectedDefaultSearchProvider(google.get());
    ASSERT_FALSE(IsBraveSearchDefault());
  }

  bool IsBraveSearchDefault() {
    auto* template_url = service()->GetDefaultSearchProvider();
    if (!template_url)
      return false;
    return template_url->prepopulate_id() ==
           TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE;
  }

  WebDiscoveryCTAState GetCurrentCTAState() {
    return GetWebDiscoveryCTAState(prefs(), GetWebDiscoveryCurrentCTAId());
  }

  bool ShouldShowWebDiscoveryInfoBar() {
    return ::ShouldShowWebDiscoveryInfoBar(service(), prefs(),
                                           GetCurrentCTAState(), &test_clock_);
  }

  TemplateURLServiceTestUtil* test_util() { return test_util_.get(); }
  TemplateURLService* service() { return test_util()->model(); }
  PrefService* prefs() { return test_util()->profile()->GetPrefs(); }
  Profile* profile() { return test_util()->profile(); }
  content::WebContents* web_contents() { return web_contents_.get(); }

  base::SimpleTestClock test_clock_;
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple test_local_state_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  std::unique_ptr<TemplateURLServiceTestUtil> test_util_;
  std::unique_ptr<content::WebContents> web_contents_;
};

TEST_F(WebDiscoveryCTATest, InitialDataTest) {
  EXPECT_FALSE(prefs()->GetBoolean(kWebDiscoveryEnabled));
  const auto& info_value = prefs()->GetDict(kWebDiscoveryCTAState);
  EXPECT_TRUE(info_value.empty());

  // Tab helper can be created by default.
  WebDiscoveryTabHelper::MaybeCreateForWebContents(web_contents());
  EXPECT_TRUE(tab_helper());
}

TEST_F(WebDiscoveryCTATest, ShouldCreateTabHelperWithPrivateProfileTest) {
  // We don't need tab helper for private profile.
  auto* private_profile = profile()->GetOffTheRecordProfile(
      Profile::OTRProfileID::CreateUniqueForTesting(), true);
  auto web_contents = content::WebContentsTester::CreateTestWebContents(
      private_profile, nullptr);
  ASSERT_TRUE(web_contents.get());

  WebDiscoveryTabHelper::MaybeCreateForWebContents(web_contents.get());
  // Check helper is not attached.
  EXPECT_FALSE(WebDiscoveryTabHelper::FromWebContents(web_contents.get()));
}

TEST_F(WebDiscoveryCTATest, ShouldShowInfoBarTest) {
  // Test with search provider change.
  // Set Non as a default provider.
  SetNonBraveSearchAsDefaultProvider();
  EXPECT_FALSE(ShouldShowWebDiscoveryInfoBar());

  // Set Brave as a default provider.
  SetBraveSearchAsDefaultProvider();
  EXPECT_TRUE(ShouldShowWebDiscoveryInfoBar());

  // Don't show if already enabled.
  prefs()->SetBoolean(kWebDiscoveryEnabled, true);
  EXPECT_FALSE(ShouldShowWebDiscoveryInfoBar());

  prefs()->SetBoolean(kWebDiscoveryEnabled, false);
  EXPECT_TRUE(ShouldShowWebDiscoveryInfoBar());

  WebDiscoveryCTAState state = GetCurrentCTAState();
  state.count = 3;
  state.last_displayed = test_clock_.Now();
  SetWebDiscoveryCTAStateToPrefs(prefs(), state);

  // Should not show because 1day is not passed from previous display
  EXPECT_FALSE(ShouldShowWebDiscoveryInfoBar());

  // Should show after one day passed.
  test_clock_.Advance(base::Days(1));
  EXPECT_TRUE(ShouldShowWebDiscoveryInfoBar());

  state.count = 5;
  SetWebDiscoveryCTAStateToPrefs(prefs(), state);
  EXPECT_FALSE(ShouldShowWebDiscoveryInfoBar());

  state.count = 4;
  SetWebDiscoveryCTAStateToPrefs(prefs(), state);
  EXPECT_TRUE(ShouldShowWebDiscoveryInfoBar());

  // Don't show again if dismissed already.
  state.dismissed = true;
  SetWebDiscoveryCTAStateToPrefs(prefs(), state);
  EXPECT_FALSE(ShouldShowWebDiscoveryInfoBar());

  // Start new CTA when new id is set.
  GetWebDiscoveryCTAIDForTesting() = "v2";
  EXPECT_TRUE(ShouldShowWebDiscoveryInfoBar());
}

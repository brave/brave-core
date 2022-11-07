/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/web_discovery/web_discovery_tab_helper.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_test_util.h"
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

class WebDiscoveryDialogTest : public testing::Test {
 public:
  WebDiscoveryDialogTest() = default;

  void SetUp() override {
    test_util_ = std::make_unique<TemplateURLServiceTestUtil>();
    test_util_->profile()
        ->GetTestingPrefService()
        ->registry()
        ->RegisterBooleanPref(prefs::kDefaultSearchProviderByExtension, false);
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
    ASSERT_TRUE(web_contents_.get());
  }

  WebDiscoveryTabHelper* tab_helper() {
    return WebDiscoveryTabHelper::FromWebContents(web_contents());
  }

  void SetBraveSearchAsDefaultProvider() {
    // Set brave search as a default provider.
    std::unique_ptr<TemplateURL> brave = CreateTestTemplateURL(
        u"brave", "https://search.brave.com/", std::string(),
        base::Time::FromTimeT(100), false, false,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE);
    service()->SetUserSelectedDefaultSearchProvider(brave.get());
    ASSERT_TRUE(IsBraveSearchDefault());
  }

  void SetNonBraveSearchAsDefaultProvider() {
    // Set brave search as a default provider.
    std::unique_ptr<TemplateURL> google = CreateTestTemplateURL(
        u"google", "https://www.google.com/", std::string(),
        base::Time::FromTimeT(100), false, false,
        TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_GOOGLE);
    service()->SetUserSelectedDefaultSearchProvider(google.get());
  }

  bool IsBraveSearchDefault() {
    return tab_helper()->IsBraveSearchDefault(service());
  }

  TemplateURLServiceTestUtil* test_util() { return test_util_.get(); }
  TemplateURLService* service() { return test_util()->model(); }
  PrefService* prefs() { return test_util()->profile()->GetPrefs(); }
  Profile* profile() { return test_util()->profile(); }
  content::WebContents* web_contents() { return web_contents_.get(); }

  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  std::unique_ptr<TemplateURLServiceTestUtil> test_util_;
  std::unique_ptr<content::WebContents> web_contents_;
};

TEST_F(WebDiscoveryDialogTest, InitialDataTest) {
  EXPECT_FALSE(prefs()->GetBoolean(kWebDiscoveryEnabled));

  // Tab helper can be created by default.
  WebDiscoveryTabHelper::MaybeCreateForWebContents(web_contents());
  EXPECT_TRUE(tab_helper());
}

TEST_F(WebDiscoveryDialogTest, ShouldCreateTabHelperWithPrivateProfileTest) {
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

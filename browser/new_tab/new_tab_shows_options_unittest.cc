/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/new_tab_shows_options.h"

#include "brave/components/brave_new_tab/new_tab_prefs.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/url_constants.h"

using brave_new_tab::prefs::NewTabShowsOption;

class BraveNewTabTest : public testing::Test {
 public:
  BraveNewTabTest() : manager_(TestingBrowserProcess::GetGlobal()) {}
  ~BraveNewTabTest() override = default;

 protected:
  void SetUp() override {
    ASSERT_TRUE(manager_.SetUp());
  }

  TestingProfileManager* manager() { return &manager_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager manager_;
};

TEST_F(BraveNewTabTest, BasicTest) {
  Profile* profile = manager()->CreateTestingProfile("Test 1");
  Profile* otr_profile =
      profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  ASSERT_TRUE(profile);
  ASSERT_TRUE(otr_profile);

  auto* prefs = profile->GetPrefs();

  // Check NTP url is empty for DASHBOARD.
  prefs->SetInteger(brave_new_tab::prefs::kNewTabShowsOption,
                    static_cast<int>(NewTabShowsOption::kDashboard));
  EXPECT_EQ(GURL(), brave::GetNewTabPageURL(profile));
  EXPECT_EQ(GURL(), brave::GetNewTabPageURL(otr_profile));
  EXPECT_TRUE(brave::ShouldUseNewTabURLForNewTab(profile));
  EXPECT_TRUE(brave::ShouldNewTabShowDashboard(profile));

  // Check NTP url is empty when option is HOMEPAGE and kHomePageIsNewTabPage
  // is true.
  prefs->SetInteger(brave_new_tab::prefs::kNewTabShowsOption,
                    static_cast<int>(NewTabShowsOption::kHomepage));
  prefs->SetString(prefs::kHomePage, "https://www.brave.com/");
  prefs->SetBoolean(prefs::kHomePageIsNewTabPage, true);
  EXPECT_EQ(GURL(), brave::GetNewTabPageURL(profile));
  EXPECT_EQ(GURL(), brave::GetNewTabPageURL(otr_profile));
  EXPECT_TRUE(brave::ShouldUseNewTabURLForNewTab(profile));
  EXPECT_TRUE(brave::ShouldNewTabShowDashboard(profile));

  // Check NTP url is configured url when option is HOMEPAGE and
  // kHomePageIsNewTabPage is false.
  prefs->SetBoolean(prefs::kHomePageIsNewTabPage, false);
  EXPECT_EQ(GURL("https://www.brave.com/"), brave::GetNewTabPageURL(profile));
  EXPECT_EQ(GURL(), brave::GetNewTabPageURL(otr_profile));
  EXPECT_FALSE(brave::ShouldUseNewTabURLForNewTab(profile));
  EXPECT_FALSE(brave::ShouldNewTabShowDashboard(profile));

  // If homepage url is newtab url, dashboard settings should be shown.
  prefs->SetString(prefs::kHomePage, "chrome://newtab/");
  EXPECT_TRUE(brave::ShouldUseNewTabURLForNewTab(profile));
  EXPECT_TRUE(brave::ShouldNewTabShowDashboard(profile));

  // Check NTP url is used when option is BLANKPAGE.
  // Blank page will go NTP route and BraveNewTabUI will handle it.
  prefs->SetInteger(brave_new_tab::prefs::kNewTabShowsOption,
                    static_cast<int>(NewTabShowsOption::kBlankpage));
  EXPECT_EQ(GURL(), brave::GetNewTabPageURL(profile));
  EXPECT_EQ(GURL(), brave::GetNewTabPageURL(otr_profile));
  EXPECT_TRUE(brave::ShouldUseNewTabURLForNewTab(profile));
  EXPECT_FALSE(brave::ShouldNewTabShowDashboard(profile));
  EXPECT_TRUE(brave::ShouldNewTabShowBlankpage(profile));
  EXPECT_FALSE(brave::ShouldNewTabShowBlankpage(otr_profile));
}

/*  Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave_shields::BraveShieldsTabHelper;
using brave_shields::ControlType;
using brave_shields::mojom::AdBlockMode;

namespace {
constexpr char kTestProfileName[] = "TestProfile";

class MockObserver : public BraveShieldsTabHelper::Observer {
 public:
  MOCK_METHOD(void, OnResourcesChanged, (), (override));
  MOCK_METHOD(void, OnShieldsEnabledChanged, (), (override));
};
}  // namespace

class BraveShieldsDataControllerTest : public testing::Test {
 public:
  BraveShieldsDataControllerTest() = default;
  ~BraveShieldsDataControllerTest() override = default;

  BraveShieldsDataControllerTest(const BraveShieldsDataControllerTest&) =
      delete;
  BraveShieldsDataControllerTest& operator=(
      const BraveShieldsDataControllerTest&) = delete;

  void SetUp() override {
    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_ = std::make_unique<TestingProfileManager>(browser_process);
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(kTestProfileName);

    test_web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
    favicon::ContentFaviconDriver::CreateForWebContents(
        test_web_contents_.get(), nullptr);
    BraveShieldsTabHelper::CreateForWebContents(test_web_contents_.get());
  }

  void TearDown() override {
    test_web_contents_.reset();
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile(kTestProfileName);
  }

  Profile* profile() { return profile_; }
  content::WebContents* web_contents() { return test_web_contents_.get(); }

  void SetLastCommittedUrl(const GURL& url) {
    content::WebContentsTester::For(web_contents())->SetLastCommittedURL(url);
  }

  BraveShieldsTabHelper* GetShieldsDataController() {
    return BraveShieldsTabHelper::FromWebContents(web_contents());
  }

  ContentSetting GetContentSettingFor(ContentSettingsType type,
                                      GURL secondary_url = GURL()) {
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
    auto* controller = GetShieldsDataController();

    return map->GetContentSetting(controller->GetCurrentSiteURL(),
                                  secondary_url, type);
  }

  void SetContentSettingFor(ContentSettingsType type,
                            ContentSetting setting,
                            GURL secondary_url = GURL()) {
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
    auto* controller = GetShieldsDataController();

    map->SetContentSettingCustomScope(
        brave_shields::GetPatternFromURL(controller->GetCurrentSiteURL()),
        brave_shields::GetPatternFromURL(secondary_url), type, setting);
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::WebContents> test_web_contents_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  raw_ptr<Profile> profile_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

TEST_F(BraveShieldsDataControllerTest, SetAdBlockModeForOrigin1) {
  auto* controller = GetShieldsDataController();
  SetLastCommittedUrl(GURL("http://brave.com"));

  /* DEFAULT */
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_DEFAULT);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_DEFAULT);

  /* ALLOW */
  controller->SetAdBlockMode(AdBlockMode::ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);

  /* STANDARD */
  controller->SetAdBlockMode(AdBlockMode::STANDARD);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);

  /* ALLOW */
  controller->SetAdBlockMode(AdBlockMode::ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);

  /* AGGRESSIVE */
  controller->SetAdBlockMode(AdBlockMode::AGGRESSIVE);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_BLOCK);

  /* ALLOW */
  controller->SetAdBlockMode(AdBlockMode::ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);
}

TEST_F(BraveShieldsDataControllerTest, SetAdBlockModeForOrigin2) {
  auto* controller = GetShieldsDataController();
  SetLastCommittedUrl(GURL("http://brave.com"));

  /* DEFAULT */
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_DEFAULT);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_DEFAULT);

  /* STANDARD */
  controller->SetAdBlockMode(AdBlockMode::STANDARD);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);

  /* ALLOW */
  controller->SetAdBlockMode(AdBlockMode::ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);

  /* STANDARD */
  controller->SetAdBlockMode(AdBlockMode::STANDARD);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);

  /* AGGRESSIVE */
  controller->SetAdBlockMode(AdBlockMode::AGGRESSIVE);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_BLOCK);

  /* STANDARD */
  controller->SetAdBlockMode(AdBlockMode::STANDARD);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);
}

TEST_F(BraveShieldsDataControllerTest, SetAdBlockModeForOrigin3) {
  auto* controller = GetShieldsDataController();
  SetLastCommittedUrl(GURL("http://brave.com"));

  /* DEFAULT */
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_DEFAULT);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_DEFAULT);

  /* AGGRESSIVE */
  controller->SetAdBlockMode(AdBlockMode::AGGRESSIVE);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_BLOCK);

  /* ALLOW */
  controller->SetAdBlockMode(AdBlockMode::ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);

  /* AGGRESSIVE */
  controller->SetAdBlockMode(AdBlockMode::AGGRESSIVE);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_BLOCK);

  /* STANDARD */
  controller->SetAdBlockMode(AdBlockMode::STANDARD);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);

  /* AGGRESSIVE */
  controller->SetAdBlockMode(AdBlockMode::AGGRESSIVE);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_BLOCK);
}

TEST_F(BraveShieldsDataControllerTest, GetAdBlockModeForOrigin) {
  auto* controller = GetShieldsDataController();
  SetLastCommittedUrl(GURL("http://brave.com"));

  /* DEFAULT */
  EXPECT_EQ(controller->GetAdBlockMode(), AdBlockMode::STANDARD);

  /* ALLOW */
  SetContentSettingFor(ContentSettingsType::BRAVE_ADS, CONTENT_SETTING_ALLOW);
  EXPECT_EQ(controller->GetAdBlockMode(), AdBlockMode::ALLOW);

  /* STANDARD */
  SetContentSettingFor(ContentSettingsType::BRAVE_ADS, CONTENT_SETTING_BLOCK);
  SetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                       CONTENT_SETTING_BLOCK);
  SetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                       CONTENT_SETTING_ALLOW, GURL("https://firstParty/"));
  EXPECT_EQ(controller->GetAdBlockMode(), AdBlockMode::STANDARD);

  /* AGGRESSIVE */
  SetContentSettingFor(ContentSettingsType::BRAVE_ADS, CONTENT_SETTING_BLOCK);
  SetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                       CONTENT_SETTING_BLOCK);
  SetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                       CONTENT_SETTING_BLOCK, GURL("https://firstParty/"));
  EXPECT_EQ(controller->GetAdBlockMode(), AdBlockMode::AGGRESSIVE);
}

TEST_F(BraveShieldsDataControllerTest, ObserverOnShieldsEnabledChangedTest) {
  // Set url for default web contents.
  SetLastCommittedUrl(GURL("http://brave.com"));

  // Create another web contents for testing whether its
  // OnShieldsEnabledChanged() callback is called when shields enabled is
  // changed by another web contents when both loaded same url.
  MockObserver observer_2;
  EXPECT_CALL(observer_2, OnShieldsEnabledChanged).Times(1);
  auto web_contents_2 =
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
  favicon::ContentFaviconDriver::CreateForWebContents(web_contents_2.get(),
                                                      nullptr);
  BraveShieldsTabHelper::CreateForWebContents(web_contents_2.get());
  auto* ctrl_2 = BraveShieldsTabHelper::FromWebContents(web_contents_2.get());
  ctrl_2->AddObserver(&observer_2);
  content::WebContentsTester::For(web_contents_2.get())
      ->SetLastCommittedURL(GURL("http://brave.com"));

  // Create another web contents for testing whether its
  // OnShieldsEnabledChanged() callback is *not* called when shields enabled is
  // changed by another web contents when both loaded *different* url.
  MockObserver observer_3;
  EXPECT_CALL(observer_3, OnShieldsEnabledChanged).Times(0);
  auto web_contents_3 =
      content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
  favicon::ContentFaviconDriver::CreateForWebContents(web_contents_3.get(),
                                                      nullptr);
  BraveShieldsTabHelper::CreateForWebContents(web_contents_3.get());
  auto* ctrl_3 = BraveShieldsTabHelper::FromWebContents(web_contents_3.get());
  ctrl_3->AddObserver(&observer_3);
  content::WebContentsTester::For(web_contents_3.get())
      ->SetLastCommittedURL(GURL("http://github.com"));

  // Change default web contents' shields enabled setting.
  // And this changes will affect |web_contents_2| as both loaded same url.
  GetShieldsDataController()->SetBraveShieldsEnabled(false);

  ctrl_2->RemoveObserver(&observer_2);
  ctrl_3->RemoveObserver(&observer_3);
}

TEST_F(BraveShieldsDataControllerTest, SetBraveShieldsEnabledAsDefaultValue) {
  // Set url for default web contents.
  SetLastCommittedUrl(GURL("http://brave.com"));
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  EXPECT_EQ(map->GetDefaultContentSetting(ContentSettingsType::BRAVE_SHIELDS,
                                          nullptr),
            CONTENT_SETTING_ALLOW);

  EXPECT_TRUE(GetShieldsDataController()->GetBraveShieldsEnabled());
  GetShieldsDataController()->SetBraveShieldsEnabled(false);
  EXPECT_FALSE(GetShieldsDataController()->GetBraveShieldsEnabled());
  EXPECT_FALSE(profile()
                   ->GetPrefs()
                   ->GetDict("profile.content_settings.exceptions.braveShields")
                   .empty());

  GetShieldsDataController()->SetBraveShieldsEnabled(true);
  EXPECT_TRUE(GetShieldsDataController()->GetBraveShieldsEnabled());
  EXPECT_TRUE(profile()
                  ->GetPrefs()
                  ->GetDict("profile.content_settings.exceptions.braveShields")
                  .empty());
}

TEST_F(BraveShieldsDataControllerTest, AllowedOnceScripts) {
  EXPECT_EQ(GetShieldsDataController()->GetAllowedJsList().size(), 0u);
  GetShieldsDataController()->HandleItemAllowedOnce(
      brave_shields::kJavaScript, "https://url1.com/script.js");
  EXPECT_EQ(GetShieldsDataController()->GetAllowedJsList().size(), 1u);
  GetShieldsDataController()->HandleItemAllowedOnce(
      brave_shields::kJavaScript, "https://url2.com/script.js");
  EXPECT_EQ(GetShieldsDataController()->GetAllowedJsList().size(), 2u);
  GetShieldsDataController()->HandleItemAllowedOnce(
      brave_shields::kJavaScript, "https://url3.com/script.js");
  EXPECT_EQ(GetShieldsDataController()->GetAllowedJsList().size(), 3u);

  // Making sure we exclude duplicates
  GetShieldsDataController()->HandleItemAllowedOnce(
      brave_shields::kJavaScript, "https://url2.com/script.js");
  GetShieldsDataController()->HandleItemAllowedOnce(
      brave_shields::kJavaScript, "https://url3.com/script.js");
  EXPECT_EQ(GetShieldsDataController()->GetAllowedJsList().size(), 3u);

  GetShieldsDataController()->ClearAllResourcesList();
  EXPECT_EQ(GetShieldsDataController()->GetAllowedJsList().size(), 0u);
}

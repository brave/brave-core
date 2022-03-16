/*  Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/ui/brave_shields_data_controller.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/brave_shields_panel.mojom.h"
#include "brave/components/brave_shields/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave_shields::BraveShieldsDataController;
using brave_shields::ControlType;
using brave_shields::mojom::AdBlockMode;

namespace {
constexpr char kTestProfileName[] = "TestProfile";
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
    profile_manager_.reset(new TestingProfileManager(browser_process));
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(kTestProfileName);

    test_web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
    BraveShieldsDataController::CreateForWebContents(test_web_contents_.get());
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

  BraveShieldsDataController* GetShieldsDataController() {
    return BraveShieldsDataController::FromWebContents(web_contents());
  }

  ContentSetting GetContentSettingFor(ContentSettingsType type,
                                      GURL secondary_url = GURL()) {
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
    auto* controller = GetShieldsDataController();

    return map->GetContentSetting(controller->GetCurrentSiteURL(),
                                  secondary_url, type);
  }

  ContentSettingsPattern GetPatternFromURL(const GURL& url) {
    DCHECK(url.is_empty() ? url.possibly_invalid_spec() == "" : url.is_valid());
    if (url.is_empty() && url.possibly_invalid_spec() == "")
      return ContentSettingsPattern::Wildcard();
    return ContentSettingsPattern::FromString("*://" + url.host() + "/*");
  }

  void SetContentSettingFor(ContentSettingsType type,
                            ContentSetting setting,
                            GURL secondary_url = GURL()) {
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
    auto* controller = GetShieldsDataController();

    map->SetContentSettingCustomScope(
        GetPatternFromURL(controller->GetCurrentSiteURL()),
        GetPatternFromURL(secondary_url), type, setting);
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::WebContents> test_web_contents_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  Profile* profile_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

TEST_F(BraveShieldsDataControllerTest, SetAdBlockMode_ForOrigin_1) {
  auto* controller = GetShieldsDataController();
  SetLastCommittedUrl(GURL("http://brave.com"));

  /* DEFAULT */
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_DEFAULT);
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
            CONTENT_SETTING_BLOCK);
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

  /* ALLOW */
  controller->SetAdBlockMode(AdBlockMode::ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_ALLOW);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING),
            CONTENT_SETTING_BLOCK);
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                 GURL("https://firstParty/")),
            CONTENT_SETTING_ALLOW);
}

TEST_F(BraveShieldsDataControllerTest, SetAdBlockMode_ForOrigin_2) {
  auto* controller = GetShieldsDataController();
  SetLastCommittedUrl(GURL("http://brave.com"));

  /* DEFAULT */
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_DEFAULT);
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
            CONTENT_SETTING_BLOCK);
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

TEST_F(BraveShieldsDataControllerTest, SetAdBlockMode_ForOrigin_3) {
  auto* controller = GetShieldsDataController();
  SetLastCommittedUrl(GURL("http://brave.com"));

  /* DEFAULT */
  EXPECT_EQ(GetContentSettingFor(ContentSettingsType::BRAVE_ADS),
            CONTENT_SETTING_DEFAULT);
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

TEST_F(BraveShieldsDataControllerTest, GetAdBlockMode_ForOrigin) {
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

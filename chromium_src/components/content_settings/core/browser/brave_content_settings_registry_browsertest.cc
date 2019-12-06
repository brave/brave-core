/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"

const GURL kBraveURL("https://www.brave.com");

class BraveContentSettingsRegistryBrowserTest : public InProcessBrowserTest {
 public:
  using InProcessBrowserTest::InProcessBrowserTest;

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  HostContentSettingsMap* private_content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(
        browser()->profile()->GetOffTheRecordProfile());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveContentSettingsRegistryBrowserTest);
};

IN_PROC_BROWSER_TEST_F(BraveContentSettingsRegistryBrowserTest,
                       WithoutWildcardContentSetting) {
  ContentSetting brave_url_shields_setting =
      content_settings()->GetContentSetting(
          kBraveURL, kBraveURL, ContentSettingsType::PLUGINS,
          brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, brave_url_shields_setting);

  ContentSetting brave_url_shields_setting_private =
      private_content_settings()->GetContentSetting(
          kBraveURL, kBraveURL, ContentSettingsType::PLUGINS,
          brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, brave_url_shields_setting_private);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsRegistryBrowserTest,
                       WithBraveShieldsContentSetting) {
  ContentSettingsPattern brave_url_pattern =
      ContentSettingsPattern::FromURL(kBraveURL);

  content_settings()->SetContentSettingCustomScope(
      brave_url_pattern,
      brave_url_pattern,
      ContentSettingsType::PLUGINS,
      brave_shields::kBraveShields,
      CONTENT_SETTING_ALLOW);

  ContentSetting brave_url_shields_setting =
      content_settings()->GetContentSetting(
          kBraveURL, kBraveURL, ContentSettingsType::PLUGINS,
          brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, brave_url_shields_setting);

  ContentSetting brave_url_shields_setting_private =
      private_content_settings()->GetContentSetting(
          kBraveURL, kBraveURL, ContentSettingsType::PLUGINS,
          brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, brave_url_shields_setting_private);
}

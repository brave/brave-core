/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/test/browser_test.h"

#if defined(OS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

const GURL& GetBraveURL() {
  static const GURL kBraveURL("https://www.brave.com");
  return kBraveURL;
}

class BraveContentSettingsRegistryBrowserTest : public PlatformBrowserTest {
 public:
  using PlatformBrowserTest::PlatformBrowserTest;

  BraveContentSettingsRegistryBrowserTest(
      const BraveContentSettingsRegistryBrowserTest&) = delete;
  BraveContentSettingsRegistryBrowserTest& operator=(
      const BraveContentSettingsRegistryBrowserTest&) = delete;

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(
        chrome_test_utils::GetProfile(this));
  }

  HostContentSettingsMap* private_content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(
        chrome_test_utils::GetProfile(this)->GetPrimaryOTRProfile(
            /*create_if_needed=*/true));
  }
};

IN_PROC_BROWSER_TEST_F(BraveContentSettingsRegistryBrowserTest,
                       WithoutWildcardContentSetting) {
  ContentSetting brave_url_shields_setting =
      content_settings()->GetContentSetting(GetBraveURL(), GetBraveURL(),
                                            ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, brave_url_shields_setting);

  ContentSetting brave_url_shields_setting_private =
      private_content_settings()->GetContentSetting(
          GetBraveURL(), GetBraveURL(), ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, brave_url_shields_setting_private);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsRegistryBrowserTest,
                       WithBraveShieldsContentSetting) {
  ContentSettingsPattern brave_url_pattern =
      ContentSettingsPattern::FromURL(GetBraveURL());

  content_settings()->SetContentSettingCustomScope(
      brave_url_pattern, brave_url_pattern, ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_ALLOW);

  ContentSetting brave_url_shields_setting =
      content_settings()->GetContentSetting(GetBraveURL(), GetBraveURL(),
                                            ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, brave_url_shields_setting);

  ContentSetting brave_url_shields_setting_private =
      private_content_settings()->GetContentSetting(
          GetBraveURL(), GetBraveURL(), ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, brave_url_shields_setting_private);
}

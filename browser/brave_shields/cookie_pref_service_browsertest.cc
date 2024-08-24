/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/ui_test_utils.h"
#endif

using brave_shields::ControlType;

class CookiePrefServiceTest : public PlatformBrowserTest {
 public:
  CookiePrefServiceTest() = default;
  ~CookiePrefServiceTest() override = default;

  Profile* profile() { return chrome_test_utils::GetProfile(this); }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(profile());
  }

  scoped_refptr<content_settings::CookieSettings> cookie_settings() {
    return CookieSettingsFactory::GetForProfile(profile());
  }

  ContentSetting GetCookiePref() {
    return IntToContentSetting(profile()->GetPrefs()->GetInteger(
        "profile.default_content_setting_values.cookies"));
  }

  void SetThirdPartyCookiePref(bool setting) {
    profile()->GetPrefs()->SetInteger(
        prefs::kCookieControlsMode,
        static_cast<int>(
            setting ? content_settings::CookieControlsMode::kBlockThirdParty
                    : content_settings::CookieControlsMode::kOff));
  }

  void SetCookiePref(ContentSetting setting) {
    profile()->GetPrefs()->SetInteger(
        "profile.default_content_setting_values.cookies", setting);
  }
};

IN_PROC_BROWSER_TEST_F(CookiePrefServiceTest, CookieControlType_Preference) {
  // Initial state
  auto setting = brave_shields::GetCookieControlType(
      content_settings(), cookie_settings().get(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, GetCookiePref());

  // Preference -> control
  /* BLOCK */
  SetCookiePref(CONTENT_SETTING_BLOCK);
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetCookieControlType(
                content_settings(), cookie_settings().get(), GURL()));

  /* ALLOW */
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);
  EXPECT_EQ(ControlType::ALLOW,
            brave_shields::GetCookieControlType(
                content_settings(), cookie_settings().get(), GURL()));

  /* BLOCK_THIRD_PARTY */
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(true);
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY,
            brave_shields::GetCookieControlType(
                content_settings(), cookie_settings().get(), GURL()));

  // Preserve CONTENT_SETTING_SESSION_ONLY
  SetCookiePref(CONTENT_SETTING_BLOCK);
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetCookieControlType(
                content_settings(), cookie_settings().get(), GURL()));
  SetCookiePref(CONTENT_SETTING_SESSION_ONLY);
  SetThirdPartyCookiePref(false);
  EXPECT_EQ(ControlType::ALLOW,
            brave_shields::GetCookieControlType(
                content_settings(), cookie_settings().get(), GURL()));
  SetCookiePref(CONTENT_SETTING_ALLOW);
  SetThirdPartyCookiePref(false);
  EXPECT_EQ(ControlType::ALLOW,
            brave_shields::GetCookieControlType(
                content_settings(), cookie_settings().get(), GURL()));
}

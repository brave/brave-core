/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/time/time.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "chrome/browser/browsing_data/browsing_data_important_sites_util.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "url/gurl.h"

namespace {
const GURL kBraveURL("https://www.brave.com");
}  // namespace

using BraveHostContentSettingsMapTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveHostContentSettingsMapTest, BasicTest) {
  auto* profile = browser()->profile();
  auto brave_url_pattern = brave_shields::GetPatternFromURL(kBraveURL, true);
  auto* map = static_cast<BraveHostContentSettingsMap*>(
      HostContentSettingsMapFactory::GetForProfile(profile));

  // Cache default settings.
  const ContentSetting default_https_setting =
      map->GetContentSetting(kBraveURL, GURL(), ContentSettingsType::PLUGINS,
                             brave_shields::kHTTPUpgradableResources);
  const ContentSetting default_js_setting =
      map->GetContentSetting(kBraveURL, GURL(),
                             ContentSettingsType::JAVASCRIPT, "");
  const ContentSetting default_flash_setting =
      map->GetContentSetting(kBraveURL, GURL(),
                             ContentSettingsType::PLUGINS, "");

  // Set content settings for http, js and flash.
  map->SetContentSettingDefaultScope(
      kBraveURL, GURL(), ContentSettingsType::PLUGINS,
      brave_shields::kHTTPUpgradableResources, CONTENT_SETTING_ALLOW);
  map->SetContentSettingCustomScope(
      brave_url_pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, "", CONTENT_SETTING_BLOCK);
  map->SetContentSettingDefaultScope(
      kBraveURL, GURL(), ContentSettingsType::PLUGINS,
      "", CONTENT_SETTING_ALLOW);

  // Check shields settings in plugin and javascript settings are not cleared
  // by clearing site settings.
  content::BrowsingDataRemover* remover =
      content::BrowserContext::GetBrowsingDataRemover(profile);

  // API used by ClearBrowsingDataHandler::HandleClearBrowsingData() when user
  // clears browsing data.
  browsing_data_important_sites_util::Remove(
      ChromeBrowsingDataRemoverDelegate::DATA_TYPE_CONTENT_SETTINGS,
      0, browsing_data::TimePeriod(),
      content::BrowsingDataFilterBuilder::Create(
          content::BrowsingDataFilterBuilder::BLACKLIST),
      remover, base::BindOnce([]() {}));

  ContentSetting setting =
      map->GetContentSetting(kBraveURL, GURL(), ContentSettingsType::PLUGINS,
                             brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(setting, CONTENT_SETTING_ALLOW);
  EXPECT_NE(default_https_setting, setting);
  setting = map->GetContentSetting(kBraveURL, GURL(),
                                   ContentSettingsType::JAVASCRIPT, "");
  EXPECT_EQ(setting, CONTENT_SETTING_BLOCK);
  EXPECT_NE(default_js_setting, setting);
  // Check flash is only cleared.
  setting = map->GetContentSetting(kBraveURL, GURL(),
                                   ContentSettingsType::PLUGINS, "");
  EXPECT_EQ(default_flash_setting, setting);

  // Check shields settings in plugin and javascript settings are cleared
  // by clearing shields settings.
  browsing_data_important_sites_util::Remove(
      ChromeBrowsingDataRemoverDelegate::DATA_TYPE_SHIELDS_SETTINGS,
      0, browsing_data::TimePeriod(),
      content::BrowsingDataFilterBuilder::Create(
          content::BrowsingDataFilterBuilder::BLACKLIST),
      remover, base::BindOnce([]() {}));
  // Set flash setting again to check it is not cleared by shields setting
  // clearing.
  map->SetContentSettingDefaultScope(
      kBraveURL, GURL(), ContentSettingsType::PLUGINS,
      "", CONTENT_SETTING_ALLOW);

  setting =
      map->GetContentSetting(kBraveURL, GURL(), ContentSettingsType::PLUGINS,
                             brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(default_https_setting, setting);
  setting = map->GetContentSetting(kBraveURL, GURL(),
                                   ContentSettingsType::JAVASCRIPT, "");
  EXPECT_EQ(default_js_setting, setting);
  // Check flash is not cleared.
  setting = map->GetContentSetting(kBraveURL, GURL(),
                                   ContentSettingsType::PLUGINS, "");
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  EXPECT_NE(default_flash_setting, setting);
}

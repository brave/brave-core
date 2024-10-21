/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/site_settings_handler.h"

#include "brave/browser/ui/webui/settings/brave_site_settings_handler.h"

#define SiteSettingsHandler BraveSiteSettingsHandler
#include "src/chrome/browser/ui/webui/settings/site_settings_handler_unittest.cc"
#undef SiteSettingsHandler

namespace settings {

// Confirm that when the user clears unpartitioned storage, or the eTLD+1 group,
// Brave Shields Metadata is also cleared.
TEST_F(SiteSettingsHandlerTest, ClearBraveShieldMetadata) {
  const GURL urls[] = {
      GURL("https://example.com/"),
      GURL("https://www.example.com"),
      GURL("https://google.com/"),
      GURL("https://www.google.com/"),
  };

  HostContentSettingsMap* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile());

  base::Value::Dict shields_metadata;
  shields_metadata.Set("farbling_token", "123");

  // Add metadata for the hosts.
  for (const auto& url : urls) {
    host_content_settings_map->SetWebsiteSettingDefaultScope(
        url, url, ContentSettingsType::BRAVE_SHIELDS_METADATA,
        base::Value(shields_metadata.Clone()));
  }

  // Schemeful sites should be used, so overall only 2 entries should be
  // present.
  EXPECT_EQ(2U, host_content_settings_map
                    ->GetSettingsForOneType(
                        ContentSettingsType::BRAVE_SHIELDS_METADATA)
                    .size());

  // Clear at the eTLD+1 level and ensure affected origins are cleared.
  base::Value::List args;
  args.Append(GroupingKey::CreateFromEtldPlus1("example.com").Serialize());
  handler()->HandleClearSiteGroupDataAndCookies(args);
  ContentSettingsForOneType shields_metadata_settings =
      host_content_settings_map->GetSettingsForOneType(
          ContentSettingsType::BRAVE_SHIELDS_METADATA);
  EXPECT_EQ(1U, shields_metadata_settings.size());

  // Expect www.google.com to be the remaining entry.
  EXPECT_EQ(ContentSettingsPattern::FromURLToSchemefulSitePattern(urls[3]),
            shields_metadata_settings.at(0).primary_pattern);
  EXPECT_EQ(ContentSettingsPattern::Wildcard(),
            shields_metadata_settings.at(0).secondary_pattern);
  EXPECT_EQ(shields_metadata, shields_metadata_settings.at(0).setting_value);

  // Clear unpartitioned usage data, which should also affect eTLD+1.
  args.clear();
  args.Append("https://google.com/");
  handler()->HandleClearUnpartitionedUsage(args);

  // Validate the shields metadata has been cleared.
  shields_metadata_settings = host_content_settings_map->GetSettingsForOneType(
      ContentSettingsType::BRAVE_SHIELDS_METADATA);
  EXPECT_EQ(0U, shields_metadata_settings.size());
}

}  // namespace settings

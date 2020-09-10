/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/common/cookie_settings_base.h"

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/optional.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content_settings {

namespace {

constexpr char kWp[] = "https://[*.]wp.com/*";
constexpr char kWordpress[] = "https://[*.]wordpress.com/*";
constexpr char kPlaystation[] = "https://[*.]playstation.com/*";
constexpr char kSonyentertainmentnetwork[] =
    "https://[*.]sonyentertainmentnetwork.com/*";
constexpr char kTwitch[] = "https://clips.twitch.tv/embed?*";
constexpr char kReddit[] = "https://[www|old]*.reddit.com/*";
constexpr char kDiscord[] = "https://[*.]discord.com/channels/*";
constexpr char kUbisoft[] = "https://[*.]ubisoft.com/*";
constexpr char kUbi[] = "https://[*.]ubi.com/*";
constexpr char kAmericanexpress[] = "https://[*.]americanexpress.com/*";
constexpr char kAexp[] = "https://[*.]aexp-static.com/*";
constexpr char kSony[] = "https://[*.]sony.com/*";
constexpr char kGoogle[] = "https://[*.]google.com/*";
constexpr char kGoogleusercontent[] = "https://[*.]googleusercontent.com/*";

bool BraveIsAllowedThirdParty(
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin) {
  static const base::NoDestructor<
      // url -> first_party_url allow map
      std::vector<std::pair<ContentSettingsPattern,
                            ContentSettingsPattern>>> entity_list({
          {
            ContentSettingsPattern::FromString(kWp),
            ContentSettingsPattern::FromString(kWordpress)
          },
          {
            ContentSettingsPattern::FromString(kWordpress),
            ContentSettingsPattern::FromString(kWp)
          },
          {
            ContentSettingsPattern::FromString(kGoogle),
            ContentSettingsPattern::FromString(kGoogleusercontent)
          },
          {
            ContentSettingsPattern::FromString(kGoogleusercontent),
            ContentSettingsPattern::FromString(kGoogle)
          },
          {
            ContentSettingsPattern::FromString(kPlaystation),
            ContentSettingsPattern::FromString(kSonyentertainmentnetwork)
          },
          {
            ContentSettingsPattern::FromString(kSonyentertainmentnetwork),
            ContentSettingsPattern::FromString(kPlaystation)
          },
          {
            ContentSettingsPattern::FromString(kSony),
            ContentSettingsPattern::FromString(kPlaystation)
          },
          {
            ContentSettingsPattern::FromString(kPlaystation),
            ContentSettingsPattern::FromString(kSony)
          },
          {
            ContentSettingsPattern::FromString(kUbisoft),
            ContentSettingsPattern::FromString(kUbi)
          },
          {
            ContentSettingsPattern::FromString(kUbi),
            ContentSettingsPattern::FromString(kUbisoft)
          },
          {
            ContentSettingsPattern::FromString(kAmericanexpress),
            ContentSettingsPattern::FromString(kAexp)
          },
          {
            ContentSettingsPattern::FromString(kAexp),
            ContentSettingsPattern::FromString(kAmericanexpress)
          },
          {
            ContentSettingsPattern::FromString(kTwitch),
            ContentSettingsPattern::FromString(kReddit)
          },
          {
            ContentSettingsPattern::FromString(kTwitch),
            ContentSettingsPattern::FromString(kDiscord)
          }
      });

  GURL first_party_url = site_for_cookies;

  if (!first_party_url.is_valid() && top_frame_origin)
      first_party_url = top_frame_origin->GetURL();

  if (net::registry_controlled_domains::GetDomainAndRegistry(
          url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) ==
      net::registry_controlled_domains::GetDomainAndRegistry(
          first_party_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    return true;

  for (auto i = entity_list->begin();
       i != entity_list->end();
       ++i) {
    if (i->first.Matches(url) && i->second.Matches(first_party_url))
      return true;
  }

  return false;
}

}  // namespace

bool CookieSettingsBase::IsCookieAccessAllowed(
    const GURL& url, const GURL& first_party_url) const {
  return IsCookieAccessAllowed(url, first_party_url, base::nullopt);
}

bool CookieSettingsBase::IsCookieAccessAllowed(
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin) const {

  // Get content settings only - do not consider default 3rd-party blocking.
  ContentSetting setting;
  GetCookieSettingInternal(
      url, top_frame_origin ? top_frame_origin->GetURL() : site_for_cookies,
      false, nullptr, &setting);

  // Content settings should always override any defaults.
  if (!IsAllowed(setting))
    return false;

  if (BraveIsAllowedThirdParty(url, site_for_cookies, top_frame_origin))
    return true;

  return IsChromiumCookieAccessAllowed(url, site_for_cookies, top_frame_origin);
}

}  // namespace content_settings

#define IsCookieAccessAllowed IsChromiumCookieAccessAllowed
#include "../../../../../../components/content_settings/core/common/cookie_settings_base.cc"  // NOLINT
#undef IsCookieAccessAllowed

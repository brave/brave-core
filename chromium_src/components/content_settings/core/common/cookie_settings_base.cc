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
#include "net/base/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content_settings {

namespace {

bool BraveIsAllowedThirdParty(
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin) {
  static const base::NoDestructor<
      // url -> first_party_url allow map
      std::vector<std::pair<ContentSettingsPattern,
                            ContentSettingsPattern>>> entity_list({
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

// TODO(bridiver) make this a common method that can be shared with
// BraveContentSettingsAgentImpl
bool ShouldUseEphemeralStorage(
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin,
    const CookieSettingsBase* const cookie_settings) {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage))
    return false;

  if (!top_frame_origin || url::Origin::Create(url) == top_frame_origin)
    return false;

  bool block_3p = !cookie_settings->IsCookieAccessAllowed(url, site_for_cookies,
                                                          top_frame_origin);
  bool block_1p = !cookie_settings->IsCookieAccessAllowed(
      url, url, url::Origin::Create(url));

  // only use ephemeral storage for block 3p
  return block_3p && !block_1p;
}

}  // namespace

bool CookieSettingsBase::IsEphemeralCookieAccessAllowed(
    const GURL& url,
    const GURL& first_party_url) const {
  return IsEphemeralCookieAccessAllowed(url, first_party_url, base::nullopt);
}

bool CookieSettingsBase::IsEphemeralCookieAccessAllowed(
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin) const {
  if (ShouldUseEphemeralStorage(url, site_for_cookies, top_frame_origin, this))
    return true;

  return IsCookieAccessAllowed(url, site_for_cookies, top_frame_origin);
}

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

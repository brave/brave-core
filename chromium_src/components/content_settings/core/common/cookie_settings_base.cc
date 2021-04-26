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

constexpr char kWp[] = "https://[*.]wp.com/*";
constexpr char kWordpress[] = "https://[*.]wordpress.com/*";
constexpr char kPlaystation[] = "https://[*.]playstation.com/*";
constexpr char kSonyentertainmentnetwork[] =
    "https://[*.]sonyentertainmentnetwork.com/*";
constexpr char kSony[] = "https://[*.]sony.com/*";
constexpr char kGoogle[] = "https://[*.]google.com/*";
constexpr char kGoogleusercontent[] = "https://[*.]googleusercontent.com/*";
constexpr char kFacebook[] = "https://[*.]facebook.com/*";
constexpr char kInstagram[] = "https://[*.]instagram.com/*";

bool BraveIsAllowedThirdParty(const GURL& url,
                              const GURL& first_party_url,
                              const CookieSettingsBase* const cookie_settings) {
  static const base::NoDestructor<
      // url -> first_party_url allow map
      std::vector<std::pair<ContentSettingsPattern, ContentSettingsPattern>>>
      entity_list(
          {{ContentSettingsPattern::FromString(kWp),
            ContentSettingsPattern::FromString(kWordpress)},
           {ContentSettingsPattern::FromString(kWordpress),
            ContentSettingsPattern::FromString(kWp)},
           {ContentSettingsPattern::FromString(kGoogle),
            ContentSettingsPattern::FromString(kGoogleusercontent)},
           {ContentSettingsPattern::FromString(kGoogleusercontent),
            ContentSettingsPattern::FromString(kGoogle)},
           {ContentSettingsPattern::FromString(kInstagram),
            ContentSettingsPattern::FromString(kFacebook)},
           {ContentSettingsPattern::FromString(kFacebook),
            ContentSettingsPattern::FromString(kInstagram)},
           {ContentSettingsPattern::FromString(kPlaystation),
            ContentSettingsPattern::FromString(kSonyentertainmentnetwork)},
           {ContentSettingsPattern::FromString(kSonyentertainmentnetwork),
            ContentSettingsPattern::FromString(kPlaystation)},
           {ContentSettingsPattern::FromString(kSony),
            ContentSettingsPattern::FromString(kPlaystation)},
           {ContentSettingsPattern::FromString(kPlaystation),
            ContentSettingsPattern::FromString(kSony)}});

  if (net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) ==
      net::registry_controlled_domains::GetDomainAndRegistry(
          first_party_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    return true;

  for (auto i = entity_list->begin(); i != entity_list->end(); ++i) {
    if (i->first.Matches(url) && i->second.Matches(first_party_url))
      return true;
  }

  return false;
}

GURL GetFirstPartyURL(const GURL& site_for_cookies,
                      const base::Optional<url::Origin>& top_frame_origin) {
  return top_frame_origin ? top_frame_origin->GetURL() : site_for_cookies;
}

bool IsFirstPartyAccessAllowed(
    const GURL& first_party_url,
    const CookieSettingsBase* const cookie_settings) {
  ContentSetting setting = cookie_settings->GetCookieSetting(
      first_party_url, first_party_url, nullptr);
  return cookie_settings->IsAllowed(setting);
}

}  // namespace

ScopedEphemeralStorageAwareness::ScopedEphemeralStorageAwareness(
    bool* ephemeral_storage_aware)
    : ephemeral_storage_aware_auto_reset_(ephemeral_storage_aware, true) {}
ScopedEphemeralStorageAwareness::~ScopedEphemeralStorageAwareness() = default;
ScopedEphemeralStorageAwareness::ScopedEphemeralStorageAwareness(
    ScopedEphemeralStorageAwareness&& rhs) = default;
ScopedEphemeralStorageAwareness& ScopedEphemeralStorageAwareness::operator=(
    ScopedEphemeralStorageAwareness&& rhs) = default;

bool CookieSettingsBase::ShouldUseEphemeralStorage(
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin) const {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage))
    return false;

  const GURL first_party_url =
      GetFirstPartyURL(site_for_cookies, top_frame_origin);

  if (!first_party_url.is_valid())
    return false;

  if (net::registry_controlled_domains::SameDomainOrHost(
          first_party_url, url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    return false;

  bool allow_3p =
      IsCookieAccessAllowedImpl(url, site_for_cookies, top_frame_origin);
  bool allow_1p = IsFirstPartyAccessAllowed(first_party_url, this);

  // only use ephemeral storage for block 3p
  return allow_1p && !allow_3p;
}

ScopedEphemeralStorageAwareness
CookieSettingsBase::CreateScopedEphemeralStorageAwareness() const {
  return ScopedEphemeralStorageAwareness(&ephemeral_storage_aware_);
}

bool CookieSettingsBase::IsEphemeralCookieAccessAllowed(
    const GURL& url,
    const GURL& first_party_url) const {
  return IsEphemeralCookieAccessAllowed(url, first_party_url, base::nullopt);
}

bool CookieSettingsBase::IsEphemeralCookieAccessAllowed(
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin) const {
  auto scoped_ephemeral_storage_awareness =
      CreateScopedEphemeralStorageAwareness();
  return IsCookieAccessAllowed(url, site_for_cookies, top_frame_origin);
}

bool CookieSettingsBase::IsCookieAccessAllowed(
    const GURL& url,
    const GURL& first_party_url) const {
  return IsCookieAccessAllowed(url, first_party_url, base::nullopt);
}

bool CookieSettingsBase::IsCookieAccessAllowed(
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin) const {
  if (ephemeral_storage_aware_ &&
      ShouldUseEphemeralStorage(url, site_for_cookies, top_frame_origin)) {
    return true;
  }

  return IsCookieAccessAllowedImpl(url, site_for_cookies, top_frame_origin);
}

bool CookieSettingsBase::IsCookieAccessAllowedImpl(
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<url::Origin>& top_frame_origin) const {
  bool allow =
      IsChromiumCookieAccessAllowed(url, site_for_cookies, top_frame_origin);

  if (allow)
    return true;

  const GURL first_party_url =
      GetFirstPartyURL(site_for_cookies, top_frame_origin);

  if (!IsFirstPartyAccessAllowed(first_party_url, this))
    return false;

  if (BraveIsAllowedThirdParty(url, first_party_url, this))
    return true;

  return false;
}

}  // namespace content_settings

#define IsCookieAccessAllowed IsChromiumCookieAccessAllowed
#include "../../../../../../components/content_settings/core/common/cookie_settings_base.cc"  // NOLINT
#undef IsCookieAccessAllowed

// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_UTILS_H_

#include <map>
#include <string>

#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace https_upgrade_exceptions {
class HttpsUpgradeExceptionsService;
}

namespace content_settings {
class CookieSettings;
}

class GURL;
class HostContentSettingsMap;
class PrefService;

namespace brave_shields {

enum ControlType {
  ALLOW = 0,
  BLOCK,
  BLOCK_THIRD_PARTY,
  DEFAULT,
};

// List of possible blocking modes when accessing blocked websites.
enum class DomainBlockingType {
  // Don't block a website, open as is.
  kNone,
  // Proceed to a website, but use Ephemeral Storage for privacy-sensitive data
  // (cookies, etc.).
  k1PES,
  // Show an interstitial before proceeding to as website.
  kAggressive,
};

struct ShieldsSettingCounts;

ContentSettingsPattern GetPatternFromURL(const GURL& url);
std::string ControlTypeToString(ControlType type);
ControlType ControlTypeFromString(const std::string& string);

void SetBraveShieldsEnabled(HostContentSettingsMap* map,
                            bool enable,
                            const GURL& url,
                            PrefService* local_state = nullptr);
bool GetBraveShieldsEnabled(HostContentSettingsMap* map, const GURL& url);

void SetAdControlType(HostContentSettingsMap* map,
                      ControlType type,
                      const GURL& url,
                      PrefService* local_state = nullptr);
ControlType GetAdControlType(HostContentSettingsMap* map, const GURL& url);

void SetCosmeticFilteringControlType(HostContentSettingsMap* map,
                                     ControlType type,
                                     const GURL& url,
                                     PrefService* local_state = nullptr,
                                     PrefService* profile_state = nullptr);
ControlType GetCosmeticFilteringControlType(HostContentSettingsMap* map,
                                            const GURL& url);
bool IsFirstPartyCosmeticFilteringEnabled(HostContentSettingsMap* map,
                                          const GURL& url);

bool IsReduceLanguageEnabledForProfile(PrefService* pref_service);

bool ShouldDoReduceLanguage(HostContentSettingsMap* map,
                            const GURL& url,
                            PrefService* pref_service);

DomainBlockingType GetDomainBlockingType(HostContentSettingsMap* map,
                                         const GURL& url);

void SetCookieControlType(HostContentSettingsMap* map,
                          PrefService* profile_state,
                          ControlType type,
                          const GURL& url,
                          PrefService* local_state = nullptr);
ControlType GetCookieControlType(
    HostContentSettingsMap* map,
    content_settings::CookieSettings* cookie_settings,
    const GURL& url);

void SetFingerprintingControlType(HostContentSettingsMap* map,
                                  ControlType type,
                                  const GURL& url,
                                  PrefService* local_state = nullptr,
                                  PrefService* profile_state = nullptr);
ControlType GetFingerprintingControlType(HostContentSettingsMap* map,
                                         const GURL& url);
bool IsBraveShieldsManaged(PrefService* prefs,
                           HostContentSettingsMap* map,
                           GURL url);

bool IsHttpsByDefaultFeatureEnabled();
void SetHttpsUpgradeControlType(HostContentSettingsMap* map,
                                ControlType type,
                                const GURL& url,
                                PrefService* local_state = nullptr);
ControlType GetHttpsUpgradeControlType(HostContentSettingsMap* map,
                                       const GURL& url);
bool ShouldUpgradeToHttps(
    HostContentSettingsMap* map,
    const GURL& url,
    https_upgrade_exceptions::HttpsUpgradeExceptionsService*
        https_upgrade_exceptions_service);
bool ShouldForceHttps(HostContentSettingsMap* map, const GURL& url);

bool IsShowStrictFingerprintingModeEnabled();

void SetNoScriptControlType(HostContentSettingsMap* map,
                            ControlType type,
                            const GURL& url,
                            PrefService* local_state = nullptr);
ControlType GetNoScriptControlType(HostContentSettingsMap* map,
                                   const GURL& url);

void SetForgetFirstPartyStorageEnabled(HostContentSettingsMap* map,
                                       bool is_enabled,
                                       const GURL& url,
                                       PrefService* local_state = nullptr);
bool GetForgetFirstPartyStorageEnabled(HostContentSettingsMap* map,
                                       const GURL& url);

// Enables a webcompat exception for a specific URL.
void SetWebcompatEnabled(HostContentSettingsMap* map,
                         ContentSettingsType webcompat_settings_type,
                         bool enabled,
                         const GURL& url,
                         PrefService* local_state);

bool IsWebcompatEnabled(HostContentSettingsMap* map,
                        ContentSettingsType webcompat_settings_type,
                        const GURL& url);

ShieldsSettingCounts GetFPSettingCount(HostContentSettingsMap* map);
ShieldsSettingCounts GetAdsSettingCount(HostContentSettingsMap* map);

mojom::FarblingLevel GetFarblingLevel(HostContentSettingsMap* map,
                                      const GURL& primary_url);
base::Token GetFarblingToken(HostContentSettingsMap* map, const GURL& url);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_BRAVE_SHIELDS_UTILS_H_

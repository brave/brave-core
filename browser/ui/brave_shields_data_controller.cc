/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "brave/browser/ui/brave_shields_data_controller.h"

#include <string>
#include <utility>

#include "base/metrics/histogram_macros.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"
#include "net/base/url_util.h"

using net::AppendQueryParameter;

namespace {

HostContentSettingsMap* GetHostContentSettingsMap(
    content::WebContents* web_contents) {
  return HostContentSettingsMapFactory::GetForProfile(
      web_contents->GetBrowserContext());
}

constexpr char kShieldsAllowScriptOnceHistogramName[] =
    "Brave.Shields.AllowScriptOnce";

}  // namespace

namespace brave_shields {

BraveShieldsDataController::~BraveShieldsDataController() = default;

BraveShieldsDataController::BraveShieldsDataController(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<BraveShieldsDataController>(*web_contents) {
  favicon::ContentFaviconDriver::FromWebContents(web_contents)
      ->AddObserver(this);
  observation_.Observe(GetHostContentSettingsMap(web_contents));
}

void BraveShieldsDataController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame() && navigation_handle->HasCommitted() &&
      !navigation_handle->IsSameDocument()) {
    ClearAllResourcesList();
  }
}

void BraveShieldsDataController::WebContentsDestroyed() {
  favicon::ContentFaviconDriver::FromWebContents(web_contents())
      ->RemoveObserver(this);
  observation_.Reset();
}

void BraveShieldsDataController::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsTypeSet content_type_set) {
  if ((content_type_set.ContainsAllTypes() ||
       content_type_set.GetType() == ContentSettingsType::BRAVE_SHIELDS) &&
      primary_pattern.Matches(GetCurrentSiteURL())) {
    for (Observer& obs : observer_list_)
      obs.OnShieldsEnabledChanged();
  }
}

void BraveShieldsDataController::OnFaviconUpdated(
    favicon::FaviconDriver* favicon_driver,
    NotificationIconType notification_icon_type,
    const GURL& icon_url,
    bool icon_url_changed,
    const gfx::Image& image) {
  for (Observer& obs : observer_list_)
    obs.OnFaviconUpdated();
}

void BraveShieldsDataController::ReloadWebContents() {
  web_contents()->GetController().Reload(content::ReloadType::NORMAL, true);
}

void BraveShieldsDataController::ClearAllResourcesList() {
  resource_list_blocked_ads_.clear();
  resource_list_http_redirects_.clear();
  resource_list_blocked_js_.clear();
  resource_list_blocked_fingerprints_.clear();
  resource_list_allowed_once_js_.clear();

  for (Observer& obs : observer_list_)
    obs.OnResourcesChanged();
}

void BraveShieldsDataController::AddObserver(Observer* obs) {
  observer_list_.AddObserver(obs);
}

void BraveShieldsDataController::RemoveObserver(Observer* obs) {
  observer_list_.RemoveObserver(obs);
}

bool BraveShieldsDataController::HasObserver(Observer* observer) {
  return observer_list_.HasObserver(observer);
}

int BraveShieldsDataController::GetTotalBlockedCount() {
  return (resource_list_blocked_ads_.size() +
          resource_list_http_redirects_.size() +
          resource_list_blocked_js_.size() +
          resource_list_blocked_fingerprints_.size());
}

std::vector<GURL> BraveShieldsDataController::GetBlockedAdsList() {
  std::vector<GURL> blocked_ads(resource_list_blocked_ads_.begin(),
                                resource_list_blocked_ads_.end());

  return blocked_ads;
}

std::vector<GURL> BraveShieldsDataController::GetHttpRedirectsList() {
  std::vector<GURL> http_redirects(resource_list_http_redirects_.begin(),
                                   resource_list_http_redirects_.end());

  return http_redirects;
}

std::vector<GURL> BraveShieldsDataController::GetBlockedJsList() {
  std::vector<GURL> js_list(resource_list_blocked_js_.begin(),
                            resource_list_blocked_js_.end());
  return js_list;
}

std::vector<GURL> BraveShieldsDataController::GetAllowedJsList() {
  std::vector<GURL> js_list(resource_list_allowed_once_js_.begin(),
                            resource_list_allowed_once_js_.end());
  return js_list;
}

std::vector<GURL> BraveShieldsDataController::GetFingerprintsList() {
  std::vector<GURL> fingerprints_list(
      resource_list_blocked_fingerprints_.begin(),
      resource_list_blocked_fingerprints_.end());

  return fingerprints_list;
}

bool BraveShieldsDataController::GetBraveShieldsEnabled() {
  return brave_shields::GetBraveShieldsEnabled(
      GetHostContentSettingsMap(web_contents()), GetCurrentSiteURL());
}

void BraveShieldsDataController::SetBraveShieldsEnabled(bool is_enabled) {
  auto* map = GetHostContentSettingsMap(web_contents());
  if (map->GetDefaultContentSetting(ContentSettingsType::BRAVE_SHIELDS,
                                    nullptr) == is_enabled) {
    brave_shields::ResetBraveShieldsEnabled(map, GetCurrentSiteURL());
  } else {
    brave_shields::SetBraveShieldsEnabled(map, is_enabled, GetCurrentSiteURL(),
                                          g_browser_process->local_state());
  }
  ReloadWebContents();
}

GURL BraveShieldsDataController::GetCurrentSiteURL() {
  return web_contents()->GetLastCommittedURL();
}

GURL BraveShieldsDataController::GetFaviconURL(bool refresh) {
  auto url = GURL("chrome://favicon2/");
  url = AppendQueryParameter(url, "size", "16");
  url = AppendQueryParameter(url, "scaleFactor", "2x");
  url = AppendQueryParameter(url, "showFallbackMonogram", "");
  url = AppendQueryParameter(url, "pageUrl",
                             GetCurrentSiteURL().GetWithoutFilename().spec());

  if (refresh) {
    url = AppendQueryParameter(
        url, "v",
        std::to_string(base::Time::Now().InMillisecondsFSinceUnixEpoch()));
  }

  return url;
}

AdBlockMode BraveShieldsDataController::GetAdBlockMode() {
  auto* map = GetHostContentSettingsMap(web_contents());

  ControlType control_type_ad =
      brave_shields::GetAdControlType(map, GetCurrentSiteURL());

  ControlType control_type_cosmetic =
      brave_shields::GetCosmeticFilteringControlType(map, GetCurrentSiteURL());

  if (control_type_ad == ControlType::ALLOW) {
    return AdBlockMode::ALLOW;
  }

  if (control_type_cosmetic == ControlType::BLOCK) {
    return AdBlockMode::AGGRESSIVE;
  } else {
    return AdBlockMode::STANDARD;
  }
}

FingerprintMode BraveShieldsDataController::GetFingerprintMode() {
  ControlType control_type = brave_shields::GetFingerprintingControlType(
      GetHostContentSettingsMap(web_contents()), GetCurrentSiteURL());

  if (control_type == ControlType::ALLOW) {
    return FingerprintMode::ALLOW_MODE;
  } else if (control_type == ControlType::BLOCK) {
    return FingerprintMode::STRICT_MODE;
  } else {
    return FingerprintMode::STANDARD_MODE;
  }
}

CookieBlockMode BraveShieldsDataController::GetCookieBlockMode() {
  auto cookie_settings = CookieSettingsFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents()->GetBrowserContext()));

  const ControlType control_type = brave_shields::GetCookieControlType(
      GetHostContentSettingsMap(web_contents()), cookie_settings.get(),
      GetCurrentSiteURL());

  switch (control_type) {
    case ControlType::ALLOW:
      return CookieBlockMode::ALLOW;
    case ControlType::BLOCK_THIRD_PARTY:
      return CookieBlockMode::CROSS_SITE_BLOCKED;
    case ControlType::BLOCK:
      return CookieBlockMode::BLOCKED;
    case ControlType::DEFAULT:
      break;
  }
  NOTREACHED_IN_MIGRATION();
  return CookieBlockMode::BLOCKED;
}

HttpsUpgradeMode BraveShieldsDataController::GetHttpsUpgradeMode() {
  ControlType control_type = brave_shields::GetHttpsUpgradeControlType(
      GetHostContentSettingsMap(web_contents()), GetCurrentSiteURL());
  if (control_type == ControlType::ALLOW) {
    return HttpsUpgradeMode::DISABLED_MODE;
  } else if (control_type == ControlType::BLOCK) {
    return HttpsUpgradeMode::STRICT_MODE;
  } else if (control_type == ControlType::BLOCK_THIRD_PARTY) {
    return HttpsUpgradeMode::STANDARD_MODE;
  } else {
    return HttpsUpgradeMode::STANDARD_MODE;
  }
}

bool BraveShieldsDataController::GetNoScriptEnabled() {
  ControlType control_type = brave_shields::GetNoScriptControlType(
      GetHostContentSettingsMap(web_contents()), GetCurrentSiteURL());

  if (control_type == ControlType::ALLOW) {
    return false;
  }

  return true;
}

bool BraveShieldsDataController::GetForgetFirstPartyStorageEnabled() {
  return brave_shields::GetForgetFirstPartyStorageEnabled(
      GetHostContentSettingsMap(web_contents()), GetCurrentSiteURL());
}

void BraveShieldsDataController::SetAdBlockMode(AdBlockMode mode) {
  auto* map = GetHostContentSettingsMap(web_contents());

  ControlType control_type_ad;
  ControlType control_type_cosmetic;

  if (mode == AdBlockMode::ALLOW) {
    control_type_ad = ControlType::ALLOW;
  } else {
    control_type_ad = ControlType::BLOCK;
  }

  if (mode == AdBlockMode::AGGRESSIVE) {
    control_type_cosmetic = ControlType::BLOCK;  // aggressive
  } else if (mode == AdBlockMode::STANDARD) {
    control_type_cosmetic = ControlType::BLOCK_THIRD_PARTY;  // standard
  } else {
    control_type_cosmetic = ControlType::ALLOW;  // allow
  }

  brave_shields::SetAdControlType(map, control_type_ad, GetCurrentSiteURL(),
                                  g_browser_process->local_state());

  PrefService* profile_prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())
          ->GetPrefs();
  brave_shields::SetCosmeticFilteringControlType(
      map, control_type_cosmetic, GetCurrentSiteURL(),
      g_browser_process->local_state(), profile_prefs);

  ReloadWebContents();
}

void BraveShieldsDataController::SetFingerprintMode(FingerprintMode mode) {
  ControlType control_type;

  if (mode == FingerprintMode::ALLOW_MODE) {
    control_type = ControlType::ALLOW;
  } else if (mode == FingerprintMode::STRICT_MODE) {
    control_type = ControlType::BLOCK;
  } else {
    control_type = ControlType::DEFAULT;  // STANDARD_MODE
  }

  PrefService* profile_prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())
          ->GetPrefs();
  brave_shields::SetFingerprintingControlType(
      GetHostContentSettingsMap(web_contents()), control_type,
      GetCurrentSiteURL(), g_browser_process->local_state(), profile_prefs);

  ReloadWebContents();
}

void BraveShieldsDataController::SetCookieBlockMode(CookieBlockMode mode) {
  auto* prefs = Profile::FromBrowserContext(web_contents()->GetBrowserContext())
                    ->GetPrefs();
  ControlType control_type = ControlType::BLOCK;

  switch (mode) {
    case CookieBlockMode::ALLOW:
      control_type = ControlType::ALLOW;
      break;
    case CookieBlockMode::CROSS_SITE_BLOCKED:
      control_type = ControlType::BLOCK_THIRD_PARTY;
      break;
    case CookieBlockMode::BLOCKED:
      control_type = ControlType::BLOCK;
      break;
  }

  brave_shields::SetCookieControlType(GetHostContentSettingsMap(web_contents()),
                                      prefs, control_type, GetCurrentSiteURL(),
                                      g_browser_process->local_state());

  ReloadWebContents();
}

void BraveShieldsDataController::SetHttpsUpgradeMode(HttpsUpgradeMode mode) {
  ControlType control_type;
  if (mode == HttpsUpgradeMode::DISABLED_MODE) {
    control_type = ControlType::ALLOW;
  } else if (mode == HttpsUpgradeMode::STRICT_MODE) {
    control_type = ControlType::BLOCK;
  } else if (mode == HttpsUpgradeMode::STANDARD_MODE) {
    control_type = ControlType::BLOCK_THIRD_PARTY;
  } else {
    control_type = ControlType::DEFAULT;
  }
  brave_shields::SetHttpsUpgradeControlType(
      GetHostContentSettingsMap(web_contents()), control_type,
      GetCurrentSiteURL(), g_browser_process->local_state());

  ReloadWebContents();
}

void BraveShieldsDataController::SetIsNoScriptEnabled(bool is_enabled) {
  ControlType control_type;

  if (!is_enabled) {
    control_type = ControlType::ALLOW;
  } else {
    control_type = ControlType::BLOCK;
  }

  brave_shields::SetNoScriptControlType(
      GetHostContentSettingsMap(web_contents()), control_type,
      GetCurrentSiteURL(), g_browser_process->local_state());

  ReloadWebContents();
}

void BraveShieldsDataController::SetForgetFirstPartyStorageEnabled(
    bool is_enabled) {
  brave_shields::SetForgetFirstPartyStorageEnabled(
      GetHostContentSettingsMap(web_contents()), is_enabled,
      GetCurrentSiteURL(), g_browser_process->local_state());
}

void BraveShieldsDataController::BlockAllowedScripts(
    const std::vector<std::string>& origins) {
  BraveShieldsWebContentsObserver* observer =
      BraveShieldsWebContentsObserver::FromWebContents(web_contents());
  if (!observer) {
    return;
  }
  observer->BlockAllowedScripts(origins);
  ReloadWebContents();
}

void BraveShieldsDataController::AllowScriptsOnce(
    const std::vector<std::string>& origins) {
  BraveShieldsWebContentsObserver* observer =
      BraveShieldsWebContentsObserver::FromWebContents(web_contents());
  if (!observer) {
    return;
  }
  UMA_HISTOGRAM_BOOLEAN(kShieldsAllowScriptOnceHistogramName, true);
  observer->AllowScriptsOnce(origins);
  ReloadWebContents();
}

bool BraveShieldsDataController::IsBraveShieldsManaged() {
  PrefService* profile_prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())
          ->GetPrefs();

  return brave_shields::IsBraveShieldsManaged(
      profile_prefs, GetHostContentSettingsMap(web_contents()),
      GetCurrentSiteURL());
}

bool BraveShieldsDataController::IsForgetFirstPartyStorageFeatureEnabled()
    const {
  return base::FeatureList::IsEnabled(
      net::features::kBraveForgetFirstPartyStorage);
}

void BraveShieldsDataController::HandleItemBlocked(
    const std::string& block_type,
    const std::string& subresource) {
  auto subres = GURL(subresource);

  if (block_type == kAds) {
    resource_list_blocked_ads_.insert(subres);
  } else if (block_type == kHTTPUpgradableResources) {
    resource_list_http_redirects_.insert(subres);
  } else if (block_type == kJavaScript) {
    resource_list_blocked_js_.insert(subres);
  } else if (block_type == kFingerprintingV2) {
    resource_list_blocked_fingerprints_.insert(subres);
  }

  for (Observer& obs : observer_list_)
    obs.OnResourcesChanged();
}

void BraveShieldsDataController::HandleItemAllowedOnce(
    const std::string& allowed_once_type,
    const std::string& subresource) {
  GURL subres(subresource);
  if (allowed_once_type != kJavaScript) {
    return;
  }
  if (resource_list_allowed_once_js_.contains(subres)) {
    return;
  }
  resource_list_allowed_once_js_.insert(std::move(subres));

  for (Observer& obs : observer_list_) {
    obs.OnResourcesChanged();
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveShieldsDataController);

}  // namespace brave_shields

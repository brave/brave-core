/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/brave_shields_tab_helper.h"

#include <string>
#include <utility>

#include "base/i18n/number_formatting.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"
#include "net/base/url_util.h"

using net::AppendQueryParameter;

namespace {

constexpr char kShieldsAllowScriptOnceHistogramName[] =
    "Brave.Shields.AllowScriptOnce";

}  // namespace

namespace brave_shields {

BraveShieldsTabHelper::~BraveShieldsTabHelper() = default;

BraveShieldsTabHelper::BraveShieldsTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<BraveShieldsTabHelper>(*web_contents),
      host_content_settings_map_(*HostContentSettingsMapFactory::GetForProfile(
          web_contents->GetBrowserContext())) {
  favicon::ContentFaviconDriver::FromWebContents(web_contents)
      ->AddObserver(this);
  observation_.Observe(&*host_content_settings_map_);
  PrefService* profile_prefs =
      Profile::FromBrowserContext(web_contents->GetBrowserContext())
          ->GetPrefs();
  brave_shields_settings_ =
      std::make_unique<brave_shields::BraveShieldsSettings>(
          *host_content_settings_map_, g_browser_process->local_state(),
          profile_prefs);
}

void BraveShieldsTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame() && navigation_handle->HasCommitted() &&
      !navigation_handle->IsSameDocument()) {
    if (navigation_handle->GetReloadType() != content::ReloadType::NORMAL) {
      // We are navigating to a new page or force-reloading. Therefore, clear
      // the webcompat features listed.
      webcompat_features_invoked_.clear();
    }
    ClearAllResourcesList();
  }
}

void BraveShieldsTabHelper::WebContentsDestroyed() {
  favicon::ContentFaviconDriver::FromWebContents(web_contents())
      ->RemoveObserver(this);
  observation_.Reset();
}

void BraveShieldsTabHelper::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsTypeSet content_type_set) {
  if ((content_type_set.ContainsAllTypes() ||
       content_type_set.GetType() == ContentSettingsType::BRAVE_SHIELDS) &&
      primary_pattern.Matches(GetCurrentSiteURL())) {
    for (Observer& obs : observer_list_) {
      obs.OnShieldsEnabledChanged();
    }
  }
}

void BraveShieldsTabHelper::OnFaviconUpdated(
    favicon::FaviconDriver* favicon_driver,
    NotificationIconType notification_icon_type,
    const GURL& icon_url,
    bool icon_url_changed,
    const gfx::Image& image) {
  for (Observer& obs : observer_list_) {
    obs.OnFaviconUpdated();
  }
}

void BraveShieldsTabHelper::ReloadWebContents() {
  web_contents()->GetController().Reload(content::ReloadType::NORMAL, true);
}

void BraveShieldsTabHelper::ClearAllResourcesList() {
  resource_list_blocked_ads_.clear();
  resource_list_http_redirects_.clear();
  resource_list_blocked_js_.clear();
  resource_list_blocked_fingerprints_.clear();
  resource_list_allowed_once_js_.clear();

  for (Observer& obs : observer_list_) {
    obs.OnResourcesChanged();
  }
}

void BraveShieldsTabHelper::AddObserver(Observer* obs) {
  observer_list_.AddObserver(obs);
}

void BraveShieldsTabHelper::RemoveObserver(Observer* obs) {
  observer_list_.RemoveObserver(obs);
}

bool BraveShieldsTabHelper::HasObserver(Observer* observer) {
  return observer_list_.HasObserver(observer);
}

int BraveShieldsTabHelper::GetTotalBlockedCount() {
  return (resource_list_blocked_ads_.size() +
          resource_list_http_redirects_.size() +
          resource_list_blocked_js_.size() +
          resource_list_blocked_fingerprints_.size());
}

std::vector<GURL> BraveShieldsTabHelper::GetBlockedAdsList() {
  std::vector<GURL> blocked_ads(resource_list_blocked_ads_.begin(),
                                resource_list_blocked_ads_.end());

  return blocked_ads;
}

std::vector<GURL> BraveShieldsTabHelper::GetHttpRedirectsList() {
  std::vector<GURL> http_redirects(resource_list_http_redirects_.begin(),
                                   resource_list_http_redirects_.end());

  return http_redirects;
}

std::vector<GURL> BraveShieldsTabHelper::GetBlockedJsList() {
  std::vector<GURL> js_list(resource_list_blocked_js_.begin(),
                            resource_list_blocked_js_.end());
  return js_list;
}

std::vector<GURL> BraveShieldsTabHelper::GetAllowedJsList() {
  std::vector<GURL> js_list(resource_list_allowed_once_js_.begin(),
                            resource_list_allowed_once_js_.end());
  return js_list;
}

std::vector<GURL> BraveShieldsTabHelper::GetFingerprintsList() {
  std::vector<GURL> fingerprints_list(
      resource_list_blocked_fingerprints_.begin(),
      resource_list_blocked_fingerprints_.end());

  return fingerprints_list;
}

const base::flat_set<ContentSettingsType>&
BraveShieldsTabHelper::GetInvokedWebcompatFeatures() {
  return webcompat_features_invoked_;
}

bool BraveShieldsTabHelper::GetBraveShieldsEnabled() {
  return brave_shields_settings_->GetBraveShieldsEnabled(GetCurrentSiteURL());
}

void BraveShieldsTabHelper::SetBraveShieldsEnabled(bool is_enabled) {
  brave_shields_settings_->SetBraveShieldsEnabled(is_enabled,
                                                  GetCurrentSiteURL());
  ReloadWebContents();
}

GURL BraveShieldsTabHelper::GetCurrentSiteURL() {
  return web_contents()->GetLastCommittedURL();
}

GURL BraveShieldsTabHelper::GetFaviconURL(bool refresh) {
  auto url = GURL("chrome://favicon2/");
  url = AppendQueryParameter(url, "size", "16");
  url = AppendQueryParameter(url, "scaleFactor", "2x");
  url = AppendQueryParameter(url, "showFallbackMonogram", "");
  url = AppendQueryParameter(url, "pageUrl",
                             GetCurrentSiteURL().GetWithoutFilename().spec());

  if (refresh) {
    url = AppendQueryParameter(
        url, "v",
        base::UTF16ToUTF8(base::FormatNumber(
            base::Time::Now().InMillisecondsFSinceUnixEpoch())));
  }

  return url;
}

AdBlockMode BraveShieldsTabHelper::GetAdBlockMode() {
  return brave_shields_settings_->GetAdBlockMode(GetCurrentSiteURL());
}

FingerprintMode BraveShieldsTabHelper::GetFingerprintMode() {
  return brave_shields_settings_->GetFingerprintMode(GetCurrentSiteURL());
}

CookieBlockMode BraveShieldsTabHelper::GetCookieBlockMode() {
  auto cookie_settings = CookieSettingsFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents()->GetBrowserContext()));

  const ControlType control_type = brave_shields::GetCookieControlType(
      &*host_content_settings_map_, cookie_settings.get(), GetCurrentSiteURL());

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
  NOTREACHED() << "Unexpected value for control_type: "
               << base::to_underlying(control_type);
}

HttpsUpgradeMode BraveShieldsTabHelper::GetHttpsUpgradeMode() {
  ControlType control_type = brave_shields::GetHttpsUpgradeControlType(
      &*host_content_settings_map_, GetCurrentSiteURL());
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

bool BraveShieldsTabHelper::GetNoScriptEnabled() {
  return brave_shields_settings_->IsNoScriptEnabled(GetCurrentSiteURL());
}

bool BraveShieldsTabHelper::GetForgetFirstPartyStorageEnabled() {
  return brave_shields::GetForgetFirstPartyStorageEnabled(
      &*host_content_settings_map_, GetCurrentSiteURL());
}

void BraveShieldsTabHelper::SetAdBlockMode(AdBlockMode mode) {
  brave_shields_settings_->SetAdBlockMode(mode, GetCurrentSiteURL());

  ReloadWebContents();
}

void BraveShieldsTabHelper::SetFingerprintMode(FingerprintMode mode) {
  brave_shields_settings_->SetFingerprintMode(mode, GetCurrentSiteURL());

  ReloadWebContents();
}

void BraveShieldsTabHelper::SetCookieBlockMode(CookieBlockMode mode) {
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

  brave_shields::SetCookieControlType(&*host_content_settings_map_, prefs,
                                      control_type, GetCurrentSiteURL(),
                                      g_browser_process->local_state());

  ReloadWebContents();
}

void BraveShieldsTabHelper::SetHttpsUpgradeMode(HttpsUpgradeMode mode) {
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
  brave_shields::SetHttpsUpgradeControlType(&*host_content_settings_map_,
                                            control_type, GetCurrentSiteURL(),
                                            g_browser_process->local_state());

  ReloadWebContents();
}

void BraveShieldsTabHelper::SetIsNoScriptEnabled(bool is_enabled) {
  brave_shields_settings_->SetNoScriptEnabled(is_enabled, GetCurrentSiteURL());

  ReloadWebContents();
}

void BraveShieldsTabHelper::SetForgetFirstPartyStorageEnabled(bool is_enabled) {
  brave_shields::SetForgetFirstPartyStorageEnabled(
      &*host_content_settings_map_, is_enabled, GetCurrentSiteURL(),
      g_browser_process->local_state());
}

void BraveShieldsTabHelper::BlockAllowedScripts(
    const std::vector<std::string>& origins) {
  BraveShieldsWebContentsObserver* observer =
      BraveShieldsWebContentsObserver::FromWebContents(web_contents());
  if (!observer) {
    return;
  }
  observer->BlockAllowedScripts(origins);
  ReloadWebContents();
}

void BraveShieldsTabHelper::AllowScriptsOnce(
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

bool BraveShieldsTabHelper::IsBraveShieldsManaged() {
  PrefService* profile_prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())
          ->GetPrefs();

  return brave_shields::IsBraveShieldsManaged(
      profile_prefs, &*host_content_settings_map_, GetCurrentSiteURL());
}

bool BraveShieldsTabHelper::IsForgetFirstPartyStorageFeatureEnabled() const {
  return base::FeatureList::IsEnabled(
      net::features::kBraveForgetFirstPartyStorage);
}

void BraveShieldsTabHelper::HandleItemBlocked(const std::string& block_type,
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

  for (Observer& obs : observer_list_) {
    obs.OnResourcesChanged();
  }
}

void BraveShieldsTabHelper::HandleItemAllowedOnce(
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

void BraveShieldsTabHelper::HandleWebcompatFeatureInvoked(
    ContentSettingsType webcompat_content_settings) {
  if (webcompat_content_settings > ContentSettingsType::BRAVE_WEBCOMPAT_NONE &&
      webcompat_content_settings < ContentSettingsType::BRAVE_WEBCOMPAT_ALL) {
    webcompat_features_invoked_.insert(webcompat_content_settings);
  }

  for (Observer& obs : observer_list_) {
    obs.OnResourcesChanged();
  }
}

void BraveShieldsTabHelper::SetWebcompatEnabled(
    ContentSettingsType webcompat_settings_type,
    bool enabled) {
  brave_shields::SetWebcompatEnabled(
      &*host_content_settings_map_, webcompat_settings_type, enabled,
      GetCurrentSiteURL(), g_browser_process->local_state());
  ReloadWebContents();
}

base::flat_map<ContentSettingsType, bool>
BraveShieldsTabHelper::GetWebcompatSettings() {
  const GURL& current_site_url = GetCurrentSiteURL();
  base::flat_map<ContentSettingsType, bool> result;
  for (auto webcompat_settings_type = ContentSettingsType::BRAVE_WEBCOMPAT_NONE;
       webcompat_settings_type != ContentSettingsType::BRAVE_WEBCOMPAT_ALL;
       webcompat_settings_type = static_cast<ContentSettingsType>(
           static_cast<int32_t>(webcompat_settings_type) + 1)) {
    const bool enabled = brave_shields::IsWebcompatEnabled(
        &*host_content_settings_map_, webcompat_settings_type,
        current_site_url);
    result[webcompat_settings_type] = enabled;
  }
  return result;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveShieldsTabHelper);

}  // namespace brave_shields

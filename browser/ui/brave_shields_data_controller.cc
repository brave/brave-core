/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "brave/browser/ui/brave_shields_data_controller.h"

#include <string>

#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"

namespace brave_shields {

BraveShieldsDataController::~BraveShieldsDataController() = default;

BraveShieldsDataController::BraveShieldsDataController(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<BraveShieldsDataController>(*web_contents) {}

void BraveShieldsDataController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame() && navigation_handle->HasCommitted() &&
      !navigation_handle->IsSameDocument()) {
    ClearAllResourcesList();
  }
}

void BraveShieldsDataController::ReloadWebContents() {
  GetWebContents().GetController().Reload(content::ReloadType::NORMAL, true);
}

void BraveShieldsDataController::ClearAllResourcesList() {
  resource_list_blocked_ads_.clear();
  resource_list_http_redirects_.clear();
  resource_list_blocked_js_.clear();
  resource_list_blocked_fingerprints_.clear();

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

std::vector<GURL> BraveShieldsDataController::GetJsList() {
  std::vector<GURL> js_list(resource_list_blocked_js_.begin(),
                            resource_list_blocked_js_.end());

  return js_list;
}

std::vector<GURL> BraveShieldsDataController::GetFingerprintsList() {
  std::vector<GURL> fingerprints_list(
      resource_list_blocked_fingerprints_.begin(),
      resource_list_blocked_fingerprints_.end());

  return fingerprints_list;
}

bool BraveShieldsDataController::GetBraveShieldsEnabled() {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());
  return brave_shields::GetBraveShieldsEnabled(map, GetCurrentSiteURL());
}

GURL BraveShieldsDataController::GetCurrentSiteURL() {
  return GetWebContents().GetLastCommittedURL();
}

AdBlockMode BraveShieldsDataController::GetAdBlockMode() {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());

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
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());

  ControlType control_type =
      brave_shields::GetFingerprintingControlType(map, GetCurrentSiteURL());

  if (control_type == ControlType::ALLOW) {
    return FingerprintMode::ALLOW;
  } else if (control_type == ControlType::BLOCK) {
    return FingerprintMode::STRICT;
  } else {
    return FingerprintMode::STANDARD;
  }
}

CookieBlockMode BraveShieldsDataController::GetCookieBlockMode() {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());

  ControlType control_type =
      brave_shields::GetCookieControlType(map, GetCurrentSiteURL());

  if (control_type == ControlType::ALLOW) {
    return CookieBlockMode::ALLOW;
  } else if (control_type == ControlType::BLOCK_THIRD_PARTY) {
    return CookieBlockMode::CROSS_SITE_BLOCKED;
  } else {
    return CookieBlockMode::BLOCKED;
  }
}

bool BraveShieldsDataController::GetHTTPSEverywhereEnabled() {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());
  return brave_shields::GetHTTPSEverywhereEnabled(map, GetCurrentSiteURL());
}

bool BraveShieldsDataController::GetNoScriptEnabled() {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());
  ControlType control_type =
      brave_shields::GetNoScriptControlType(map, GetCurrentSiteURL());

  if (control_type == ControlType::ALLOW) {
    return false;
  }

  return true;
}

void BraveShieldsDataController::SetAdBlockMode(AdBlockMode mode) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());

  ControlType control_type_ad;
  ControlType control_type_cosmetic;

  if (mode == AdBlockMode::ALLOW) {
    control_type_ad = ControlType::ALLOW;
  } else {
    control_type_ad = ControlType::BLOCK;
  }

  if (mode == AdBlockMode::AGGRESSIVE) {
    control_type_cosmetic = ControlType::BLOCK;  // aggressive
  } else {
    control_type_cosmetic = ControlType::BLOCK_THIRD_PARTY;  // standard
  }

  brave_shields::SetAdControlType(map, control_type_ad, GetCurrentSiteURL(),
                                  g_browser_process->local_state());

  brave_shields::SetCosmeticFilteringControlType(
      map, control_type_cosmetic, GetCurrentSiteURL(),
      g_browser_process->local_state());

  ReloadWebContents();
}

void BraveShieldsDataController::SetFingerprintMode(FingerprintMode mode) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());

  ControlType control_type;

  if (mode == FingerprintMode::ALLOW) {
    control_type = ControlType::ALLOW;
  } else if (mode == FingerprintMode::STRICT) {
    control_type = ControlType::BLOCK;
  } else {
    control_type = ControlType::DEFAULT;  // STANDARD
  }

  brave_shields::SetFingerprintingControlType(
      map, control_type, GetCurrentSiteURL(), g_browser_process->local_state());

  ReloadWebContents();
}

void BraveShieldsDataController::SetCookieBlockMode(CookieBlockMode mode) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());

  ControlType control_type;

  if (mode == CookieBlockMode::ALLOW) {
    control_type = ControlType::ALLOW;
  } else if (mode == CookieBlockMode::CROSS_SITE_BLOCKED) {
    control_type = ControlType::BLOCK_THIRD_PARTY;
  } else {
    control_type = ControlType::BLOCK;  // STANDARD
  }

  brave_shields::SetCookieControlType(map, control_type, GetCurrentSiteURL(),
                                      g_browser_process->local_state());

  ReloadWebContents();
}

void BraveShieldsDataController::SetIsNoScriptEnabled(bool is_enabled) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());

  ControlType control_type;

  if (!is_enabled) {
    control_type = ControlType::ALLOW;
  } else {
    control_type = ControlType::BLOCK;
  }

  brave_shields::SetNoScriptControlType(map, control_type, GetCurrentSiteURL(),
                                        g_browser_process->local_state());

  ReloadWebContents();
}

void BraveShieldsDataController::SetIsHTTPSEverywhereEnabled(bool is_enabled) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(
      GetWebContents().GetBrowserContext());

  brave_shields::SetHTTPSEverywhereEnabled(map, is_enabled, GetCurrentSiteURL(),
                                           g_browser_process->local_state());

  ReloadWebContents();
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

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveShieldsDataController);

}  // namespace brave_shields

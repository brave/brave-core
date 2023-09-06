/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/https_upgrades_navigation_throttle.h"

#include "base/time/time.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/https_first_mode_settings_tracker.h"
#include "chrome/browser/ssl/https_only_mode_controller_client.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/stateful_ssl_host_state_delegate.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"

namespace {

// Tor is slow and needs a longer fallback delay
constexpr base::TimeDelta kTorFallbackDelay = base::Seconds(20);

bool IsTor(content::NavigationHandle* handle) {
  auto* context = handle->GetWebContents()->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(context);
  return profile->IsTor();
}

}  // namespace

#define MaybeCreateThrottleFor MaybeCreateThrottleFor_ChromiumImpl
#define SetNavigationTimeout(DEFAULT_TIMEOUT)                         \
  SetNavigationTimeout(IsTor(navigation_handle()) ? kTorFallbackDelay \
                                                  : DEFAULT_TIMEOUT)

#include "src/chrome/browser/ssl/https_upgrades_navigation_throttle.cc"

#undef MaybeCreateThrottleFor
#undef SetNavigationTimeout

// static
std::unique_ptr<HttpsUpgradesNavigationThrottle>
HttpsUpgradesNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* handle,
    std::unique_ptr<SecurityBlockingPageFactory> blocking_page_factory,
    Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // HTTPS-First Mode is only relevant for primary main-frame HTTP(S)
  // navigations.
  if (!handle->GetURL().SchemeIsHTTPOrHTTPS() ||
      !handle->IsInPrimaryMainFrame() || handle->IsSameDocument()) {
    return nullptr;
  }

  const GURL& request_url = handle->GetURL();
  content::BrowserContext* context =
      handle->GetWebContents()->GetBrowserContext();
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(context);

  PrefService* prefs = profile->GetPrefs();
  security_interstitials::https_only_mode::HttpInterstitialState
      interstitial_state;
  interstitial_state.enabled_by_pref =
      (prefs && prefs->GetBoolean(prefs::kHttpsOnlyModeEnabled)) ||
      (map && brave_shields::ShouldForceHttps(map, request_url));

  StatefulSSLHostStateDelegate* state =
      static_cast<StatefulSSLHostStateDelegate*>(
          profile->GetSSLHostStateDelegate());
  auto* storage_partition =
      handle->GetWebContents()->GetPrimaryMainFrame()->GetStoragePartition();

  HttpsFirstModeService* hfm_service =
      HttpsFirstModeServiceFactory::GetForProfile(profile);
  if (hfm_service) {
    // Can be null in some cases, e.g. when using Ash sign-in profile.
    hfm_service->MaybeEnableHttpsFirstModeForUrl(handle->GetURL());
  }
  // StatefulSSLHostStateDelegate can be null during tests.
  if (state && state->IsHttpsEnforcedForHost(handle->GetURL().host(),
                                             storage_partition)) {
    interstitial_state.enabled_by_engagement_heuristic = true;
  }

  bool https_upgrades_enabled =
      interstitial_state.enabled_by_pref ||
      (map && brave_shields::ShouldUpgradeToHttps(
                  map, request_url,
                  g_brave_browser_process->https_upgrade_exceptions_service()));
  if (!https_upgrades_enabled) {
    return nullptr;
  }

  // Ensure that the HttpsOnlyModeTabHelper has been created (this does nothing
  // if it has already been created for the WebContents). There are cases where
  // the tab helper won't get created by the initialization in
  // chrome/browser/ui/tab_helpers.cc but the criteria for adding the throttle
  // are still met (see crbug.com/1233889 for one example).
  HttpsOnlyModeTabHelper::CreateForWebContents(handle->GetWebContents());

  return std::make_unique<HttpsUpgradesNavigationThrottle>(
      handle, profile, std::move(blocking_page_factory), interstitial_state);
}

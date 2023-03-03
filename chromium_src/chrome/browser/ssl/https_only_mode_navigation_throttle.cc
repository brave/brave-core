/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/https_only_mode_navigation_throttle.h"

#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/https_only_mode_controller_client.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/stateful_ssl_host_state_delegate.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace {

// Tor is slow and needs a longer fallback delay
constexpr base::TimeDelta kTorFallbackDelay = base::Seconds(20);

bool ShouldUpgradeToHttps(content::NavigationHandle* handle) {
  if (!brave_shields::IsHttpsByDefaultFeatureEnabled()) {
    return false;
  }
  content::BrowserContext* context =
      handle->GetWebContents()->GetBrowserContext();
  const GURL& url = handle->GetURL();
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(context);
  return brave_shields::ShouldUpgradeToHttps(
      map, url, g_brave_browser_process->https_upgrade_exceptions_service());
}

bool IsTor(content::NavigationHandle* handle) {
  auto* context = handle->GetWebContents()->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(context);
  return profile->IsTor();
}

}  // namespace

#define WillFailRequest WillFailRequest_ChromiumImpl
#define GetBoolean(PREF_NAME) \
  GetBooleanOr(PREF_NAME, ShouldUpgradeToHttps(handle))
#define SetNavigationTimeout(DEFAULT_TIMEOUT)                         \
  SetNavigationTimeout(IsTor(navigation_handle()) ? kTorFallbackDelay \
                                                  : DEFAULT_TIMEOUT)

#include "src/chrome/browser/ssl/https_only_mode_navigation_throttle.cc"

#undef WillFailRequest
#undef GetBoolean
#undef SetNavigationTimeout

// Called if there is a non-OK net::Error in the completion status.
content::NavigationThrottle::ThrottleCheckResult
HttpsOnlyModeNavigationThrottle::WillFailRequest() {
  // Don't fall back if the HTTPS by Default feature flag
  // is disabled.
  if (!brave_shields::IsHttpsByDefaultFeatureEnabled()) {
    return WillFailRequest_ChromiumImpl();
  }

  auto* handle = navigation_handle();
  auto* contents = handle->GetWebContents();
  const GURL& request_url = handle->GetURL();

  // Don't fall back if we are using HTTPS-Only Mode or the HTTPS-Upgrade
  // Strict setting is enabled.
  content::BrowserContext* context =
      handle->GetWebContents()->GetBrowserContext();
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(context);
  Profile* profile = Profile::FromBrowserContext(context);
  auto* prefs = profile->GetPrefs();
  if ((prefs && prefs->GetBoolean(prefs::kHttpsOnlyModeEnabled)) ||
      brave_shields::ShouldForceHttps(map, request_url)) {
    return WillFailRequest_ChromiumImpl();
  }

  // Fall back only on TLS-related errors.
  if (handle->GetSSLInfo() &&
      !net::IsCertStatusError(handle->GetSSLInfo()->cert_status) &&
      handle->GetNetErrorCode() == net::OK) {
    return content::NavigationThrottle::PROCEED;
  }

  // Fall back only if the interceptor upgraded this navigation.
  auto* tab_helper = HttpsOnlyModeTabHelper::FromWebContents(contents);
  if (!tab_helper->is_navigation_upgraded()) {
    return content::NavigationThrottle::PROCEED;
  }

  // We are going to fall back.
  tab_helper->set_is_navigation_upgraded(false);
  tab_helper->set_is_navigation_fallback(true);
  StatefulSSLHostStateDelegate* state =
      static_cast<StatefulSSLHostStateDelegate*>(
          profile->GetSSLHostStateDelegate());

  // StatefulSSLHostStateDelegate can be null during tests.
  // Remember not to upgrade this host anymore.
  if (state) {
    state->AllowHttpForHost(
        request_url.host(),
        contents->GetPrimaryMainFrame()->GetStoragePartition());
  }

  // Setup the fallback task URL parameters.
  content::OpenURLParams url_params =
      content::OpenURLParams::FromNavigationHandle(handle);
  // Delete the redirect chain that tried to upgrade but hit a dead end.
  url_params.redirect_chain.clear();
  // Use the original fallback URL.
  url_params.url = tab_helper->fallback_url();
  // Launch a new task to navigate to the fallback URL.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<content::WebContents> contents,
                        const content::OpenURLParams& url_params) {
                       if (!contents) {
                         return;
                       }
                       // This deletes the NavigationThrottle and
                       // NavigationHandle.
                       contents->Stop();
                       // Navigate to the fallback URL.
                       contents->OpenURL(url_params);
                     },
                     contents->GetWeakPtr(), std::move(url_params)));
  // The NavigationThrottle object will be deleted; nothing more to do here.
  return content::NavigationThrottle::CANCEL_AND_IGNORE;
}

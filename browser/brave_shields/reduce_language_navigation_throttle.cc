/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/reduce_language_navigation_throttle.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/browser/brave_farbling_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_util.h"

namespace brave_shields {

// static
std::unique_ptr<ReduceLanguageNavigationThrottle>
ReduceLanguageNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    HostContentSettingsMap* content_settings) {
  content::BrowserContext* context =
      navigation_handle->GetWebContents()->GetBrowserContext();
  PrefService* pref_service = user_prefs::UserPrefs::Get(context);
  if (!IsReduceLanguageEnabledForProfile(pref_service))
    return nullptr;
  return std::make_unique<ReduceLanguageNavigationThrottle>(navigation_handle,
                                                            content_settings);
}

ReduceLanguageNavigationThrottle::ReduceLanguageNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    HostContentSettingsMap* content_settings)
    : content::NavigationThrottle(navigation_handle),
      content_settings_(content_settings) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

ReduceLanguageNavigationThrottle::~ReduceLanguageNavigationThrottle() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

content::NavigationThrottle::ThrottleCheckResult
ReduceLanguageNavigationThrottle::WillStartRequest() {
  UpdateHeaders();
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
ReduceLanguageNavigationThrottle::WillRedirectRequest() {
  UpdateHeaders();
  return content::NavigationThrottle::PROCEED;
}

void ReduceLanguageNavigationThrottle::UpdateHeaders() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::NavigationHandle* handle = navigation_handle();
  GURL url = handle->GetURL();
  content::BrowserContext* context =
      handle->GetWebContents()->GetBrowserContext();
  PrefService* pref_service = user_prefs::UserPrefs::Get(context);

  if (!brave_shields::ShouldDoReduceLanguage(content_settings_, url,
                                             pref_service))
    return;

  ControlType fingerprinting_control_type =
      brave_shields::GetFingerprintingControlType(content_settings_, url);

  // If fingerprint blocking is maximum, set Accept-Language header to
  // static value regardless of other preferences.
  if (fingerprinting_control_type == ControlType::BLOCK) {
    handle->SetRequestHeader(net::HttpRequestHeaders::kAcceptLanguage,
                             "en-US,en;q=0.9");
    return;
  }

  // If fingerprint blocking is default, compute Accept-Language header
  // based on user preferences.
  std::string languages =
      pref_service->Get(language::prefs::kAcceptLanguages)->GetString();
  std::string first_language = language::GetFirstLanguage(languages);
  // Add a fake q value after the language code.
  std::vector<std::string> q_values = {";q=0.5", ";q=0.6", ";q=0.7", ";q=0.8",
                                       ";q=0.9"};
  brave::FarblingPRNG prng;
  auto* profile = Profile::FromBrowserContext(context);
  if (g_brave_browser_process->brave_farbling_service()
          ->MakePseudoRandomGeneratorForURL(
              url, profile && !profile->IsOffTheRecord(), &prng)) {
    first_language += q_values[prng() % q_values.size()];
  }
  handle->SetRequestHeader(net::HttpRequestHeaders::kAcceptLanguage,
                           first_language);
}

const char* ReduceLanguageNavigationThrottle::GetNameForLogging() {
  return "ReduceLanguageNavigationThrottle";
}

}  // namespace brave_shields

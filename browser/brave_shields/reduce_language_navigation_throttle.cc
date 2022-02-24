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
#include "net/http/http_util.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_shields {

// static
std::unique_ptr<ReduceLanguageNavigationThrottle>
ReduceLanguageNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle,
    HostContentSettingsMap* content_settings) {
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kBraveReduceLanguage))
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
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  UpdateHeaders();
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
ReduceLanguageNavigationThrottle::WillRedirectRequest() {
  return WillStartRequest();
}

void ReduceLanguageNavigationThrottle::UpdateHeaders() {
  content::NavigationHandle* handle = navigation_handle();
  GURL visible_url = handle->GetWebContents()->GetVisibleURL();
  ControlType fingerprinting_control_type =
      brave_shields::GetFingerprintingControlType(content_settings_,
                                                  visible_url);
  if (fingerprinting_control_type == ControlType::ALLOW)
    return;

  if (fingerprinting_control_type == ControlType::BLOCK) {
    handle->SetRequestHeader(net::HttpRequestHeaders::kAcceptLanguage,
                             "en-US,en");
    return;
  }

  content::BrowserContext* context =
      handle->GetWebContents()->GetBrowserContext();
  PrefService* pref_service = user_prefs::UserPrefs::Get(context);
  std::string languages =
      pref_service->Get(language::prefs::kAcceptLanguages)->GetString();
  std::string first_language = language::GetFirstLanguage(languages);
  std::vector<std::string> q_values = {";q=0.5", ";q=0.6", ";q=0.7",
                                       ";q=0.8", ";q=0.9", ""};
  std::mt19937_64 prng;
  auto* profile = Profile::FromBrowserContext(context);
  if (g_brave_browser_process->MakePseudoRandomGenerator(
          visible_url, profile && !profile->IsOffTheRecord(), &prng)) {
    first_language += q_values[prng() % q_values.size()];
  }
  handle->SetRequestHeader(net::HttpRequestHeaders::kAcceptLanguage,
                           first_language);
}

const char* ReduceLanguageNavigationThrottle::GetNameForLogging() {
  return "ReduceLanguageNavigationThrottle";
}

}  // namespace brave_shields

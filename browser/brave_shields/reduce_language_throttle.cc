/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/reduce_language_throttle.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/strings/stringprintf.h"
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
#include "content/public/browser/web_contents.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_util.h"

namespace brave_shields {

// static
std::unique_ptr<ReduceLanguageThrottle>
ReduceLanguageThrottle::MaybeCreateThrottleFor(
    const content::WebContents::Getter& wc_getter,
    HostContentSettingsMap* content_settings) {
  LOG(ERROR) << "1";
  auto* contents = wc_getter.Run();
  if (!contents)
    return nullptr;

  if (PrefService* pref_service =
          user_prefs::UserPrefs::Get(contents->GetBrowserContext())) {
    if (!IsReduceLanguageEnabledForProfile(pref_service))
      return nullptr;
  }

  return std::make_unique<ReduceLanguageThrottle>(wc_getter, content_settings);
}

ReduceLanguageThrottle::ReduceLanguageThrottle(
    const content::WebContents::Getter& wc_getter,
    HostContentSettingsMap* content_settings)
    : wc_getter_(wc_getter), content_settings_(content_settings) {}

ReduceLanguageThrottle::~ReduceLanguageThrottle() = default;

void ReduceLanguageThrottle::WillStartRequest(network::ResourceRequest* request,
                                              bool* defer) {
  LOG(ERROR) << request->url;
  auto* web_contents = wc_getter_.Run();
  if (!web_contents)
    return;
  auto* browser_context = web_contents->GetBrowserContext();
  if (!browser_context)
    return;
  PrefService* pref_service = user_prefs::UserPrefs::Get(browser_context);
  if (!pref_service)
    return;
  GURL visible_url = web_contents->GetVisibleURL();
  LOG(ERROR) << "visible URL=" << visible_url;
  if (!brave_shields::ShouldDoReduceLanguage(content_settings_, visible_url,
                                             pref_service)) {
      LOG(ERROR) << "no farbling";
    return;
  }
  LOG(ERROR) << "yes farbling";
  ControlType fingerprinting_control_type =
      brave_shields::GetFingerprintingControlType(content_settings_,
                                                  visible_url);

  // If fingerprint blocking is maximum, set Accept-Language header to
  // static value regardless of other preferences.
  if (fingerprinting_control_type == ControlType::BLOCK) {
      LOG(ERROR) << "max farbling";
    request->headers.SetHeader(net::HttpRequestHeaders::kAcceptLanguage,
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
  auto* profile = Profile::FromBrowserContext(browser_context);
  if (g_brave_browser_process->brave_farbling_service()
          ->MakePseudoRandomGeneratorForURL(
              visible_url, profile && !profile->IsOffTheRecord(), &prng)) {
      LOG(ERROR) << "default farbling";
    first_language += q_values[prng() % q_values.size()];
  }
  request->headers.SetHeader(net::HttpRequestHeaders::kAcceptLanguage,
                             first_language);
  LOG(ERROR) << request->headers.ToString();
}

}  // namespace brave_shields

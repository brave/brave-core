/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "brave/browser/net/brave_reduce_language_network_delegate_helper.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/browser/brave_farbling_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "net/base/net_errors.h"

using brave_shields::ControlType;

namespace brave {

int OnBeforeStartTransaction_ReduceLanguageWork(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  auto* pref_service = user_prefs::UserPrefs::Get(ctx->browser_context);
  Profile* profile = Profile::FromBrowserContext(ctx->browser_context);
  auto* content_settings =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!brave_shields::ShouldDoReduceLanguage(content_settings, ctx->tab_origin,
                                             pref_service)) {
    return net::OK;
  }

  ControlType fingerprinting_control_type =
      brave_shields::GetFingerprintingControlType(content_settings,
                                                  ctx->tab_origin);

  // If fingerprint blocking is maximum, set Accept-Language header to
  // static value regardless of other preferences.
  std::string accept_language_string = "en-US,en;q=0.9";

  // If fingerprint blocking is default, compute Accept-Language header
  // based on user preferences.
  if (fingerprinting_control_type != ControlType::BLOCK) {
    std::string languages =
        pref_service->Get(language::prefs::kAcceptLanguages)->GetString();
    accept_language_string = language::GetFirstLanguage(languages);
    // Add a fake q value after the language code.
    std::vector<std::string> q_values = {";q=0.5", ";q=0.6", ";q=0.7", ";q=0.8",
                                         ";q=0.9"};
    brave::FarblingPRNG prng;
    if (g_brave_browser_process->brave_farbling_service()
            ->MakePseudoRandomGeneratorForURL(
                ctx->tab_origin, profile && !profile->IsOffTheRecord(),
                &prng)) {
      accept_language_string += q_values[prng() % q_values.size()];
    }
  }

  headers->SetHeader(net::HttpRequestHeaders::kAcceptLanguage,
                     accept_language_string);
  ctx->set_headers.insert(net::HttpRequestHeaders::kAcceptLanguage);

  return net::OK;
}

}  // namespace brave

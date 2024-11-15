/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_reduce_language_network_delegate_helper.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_split.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/brave_farbling_service_factory.h"
#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "net/base/net_errors.h"

using brave_shields::ControlType;

namespace brave {

namespace {
constexpr char kAcceptLanguageMax[] = "en-US,en;q=0.9";
const std::array<std::string, 5> kFakeQValues = {";q=0.5", ";q=0.6", ";q=0.7",
                                                 ";q=0.8", ";q=0.9"};
static constexpr auto kFarbleAcceptLanguageExceptions =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {
            // https://github.com/brave/brave-browser/issues/26325
            "aeroplan.rewardops.com",
            // https://github.com/brave/brave-browser/issues/31196
            "login.live.com",
            // https://github.com/brave/brave-browser/issues/25309
            "ulta.com",
            "www.ulta.com",
        });
}  // namespace

std::string FarbleAcceptLanguageHeader(
    const GURL& origin_url,
    Profile* profile,
    HostContentSettingsMap* content_settings) {
  std::string languages = profile->GetPrefs()
                              ->GetValue(language::prefs::kAcceptLanguages)
                              .GetString();
  std::string accept_language_string = language::GetFirstLanguage(languages);
  // If the first language is a multi-part code like "en-US" or "zh-HK",
  // extract and append the base language code to |accept_language_string|.
  const std::vector<std::string> tokens = base::SplitString(
      accept_language_string, "-", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (!tokens.empty() && tokens[0] != accept_language_string) {
    accept_language_string += "," + tokens[0];
  }
  // Add a fake q value after the language code.
  brave::FarblingPRNG prng;

  auto* brave_farbling_service =
      BraveFarblingServiceFactory::GetForProfile(profile);
  if (brave_farbling_service &&
      brave_farbling_service->MakePseudoRandomGeneratorForURL(origin_url,
                                                              &prng)) {
    accept_language_string += kFakeQValues[prng() % kFakeQValues.size()];
  }

  return accept_language_string;
}

int OnBeforeStartTransaction_ReduceLanguageWork(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  Profile* profile = Profile::FromBrowserContext(ctx->browser_context);
  DCHECK(profile);
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(profile);
  DCHECK(content_settings);
  GURL origin_url(ctx->tab_origin);
  if (origin_url.is_empty()) {
    origin_url = ctx->initiator_url;
  }
  if (origin_url.is_empty()) {
    return net::OK;
  }
  if (!brave_shields::ShouldDoReduceLanguage(content_settings, origin_url,
                                             profile->GetPrefs())) {
    return net::OK;
  }
  std::string_view origin_host(origin_url.host_piece());
  if (kFarbleAcceptLanguageExceptions.contains(origin_host)) {
    return net::OK;
  }

  if (headers->HasHeader(net::HttpRequestHeaders::kAcceptLanguage)) {
    // For virtually all requests (HTML, CSS, JS, images, XHR), this header will
    // not exist yet. If the request headers already include an Accept-Language
    // value here, it means something explicitly set it, e.g. a page script
    // initiating an XHR with an explicit Accept-Language header. If so, we need
    // to leave it alone, because there are a lot of servers out there that do
    // not like the Accept-Language being anything other than what their
    // client-side code set.
    // https://github.com/brave/brave-browser/issues/28945
    return net::OK;
  }

  std::string accept_language_string;
  switch (brave_shields::GetFingerprintingControlType(content_settings,
                                                      origin_url)) {
    case ControlType::BLOCK: {
      // If fingerprint blocking is maximum, set Accept-Language header to
      // static value regardless of other preferences.
      accept_language_string = kAcceptLanguageMax;
      break;
    }
    case ControlType::DEFAULT: {
      // If fingerprint blocking is default, compute Accept-Language header
      // based on user preferences and some randomization.
      accept_language_string =
          FarbleAcceptLanguageHeader(origin_url, profile, content_settings);
      break;
    }
    default:
      // Other cases are handled within ShouldDoReduceLanguage, so we should
      // never reach here.
      NOTREACHED();
  }

  headers->SetHeader(net::HttpRequestHeaders::kAcceptLanguage,
                     accept_language_string);
  ctx->set_headers.insert(net::HttpRequestHeaders::kAcceptLanguage);

  return net::OK;
}

}  // namespace brave

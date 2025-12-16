// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/static_redirect_helper/static_redirect_helper.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

#include "base/no_destructor.h"
#include "brave/components/constants/network_constants.h"
#include "brave/components/geolocation/brave_geolocation_buildflags.h"
#include "brave/components/safebrowsing/buildflags.h"
#include "extensions/common/url_pattern.h"

namespace brave {

namespace {

bool g_safebrowsing_api_endpoint_for_testing_ = false;

std::string_view GetSafeBrowsingEndpoint() {
  if (g_safebrowsing_api_endpoint_for_testing_) {
    return kSafeBrowsingTestingEndpoint;
  }
  return BUILDFLAG(SAFEBROWSING_ENDPOINT);
}

}  // namespace

void SetSafeBrowsingEndpointForTesting(bool testing) {
  g_safebrowsing_api_endpoint_for_testing_ = testing;
}

void StaticRedirectHelper(const GURL& request_url, GURL* new_url) {
  GURL::Replacements replacements;
  static base::NoDestructor<URLPattern> geo_pattern(URLPattern::SCHEME_HTTPS,
                                                    kGeoLocationsPattern);
  static base::NoDestructor<URLPattern> safeBrowsing_pattern(
      URLPattern::SCHEME_HTTPS, kSafeBrowsingPrefix);
  static base::NoDestructor<URLPattern> safebrowsingfilecheck_pattern(
      URLPattern::SCHEME_HTTPS, kSafeBrowsingFileCheckPrefix);
  static base::NoDestructor<URLPattern> safebrowsingcrxlist_pattern(
      URLPattern::SCHEME_HTTPS, kSafeBrowsingCrxListPrefix);

  // To-Do (@jumde) - Update the naming for the variables below
  // https://github.com/brave/brave-browser/issues/10314
  static base::NoDestructor<URLPattern> crlSet_pattern1(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kCRLSetPrefix1);
  static base::NoDestructor<URLPattern> crlSet_pattern2(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kCRLSetPrefix2);
  static base::NoDestructor<URLPattern> crlSet_pattern3(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kCRLSetPrefix3);
  static base::NoDestructor<URLPattern> crlSet_pattern4(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kCRLSetPrefix4);
  static base::NoDestructor<URLPattern> autofill_pattern(
      URLPattern::SCHEME_HTTPS, kAutofillPrefix);
  static base::NoDestructor<URLPattern> favicon_pattern(
      URLPattern::SCHEME_HTTPS, "https://t0.gstatic.com/faviconV2*");
  static base::NoDestructor<URLPattern> gvt1_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, "*://*.gvt1.com/*");
  static base::NoDestructor<URLPattern> googleDl_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      "*://dl.google.com/*");

  static base::NoDestructor<URLPattern> widevine_gvt1_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kWidevineGvt1Prefix);
  static base::NoDestructor<URLPattern> widevine_google_dl_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      kWidevineGoogleDlPrefix);

  if (widevine_gvt1_pattern->MatchesURL(request_url) ||
      widevine_google_dl_pattern->MatchesURL(request_url)) {
    return;
  }

  if (geo_pattern->MatchesURL(request_url)) {
    *new_url = GURL(BUILDFLAG(GOOGLEAPIS_URL));
    return;
  }

  auto safebrowsing_endpoint = GetSafeBrowsingEndpoint();
  if (!safebrowsing_endpoint.empty() &&
      safeBrowsing_pattern->MatchesHost(request_url)) {
    replacements.SetHostStr(safebrowsing_endpoint);
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (!safebrowsing_endpoint.empty() &&
      safebrowsingfilecheck_pattern->MatchesHost(request_url)) {
    replacements.SetHostStr(kBraveSafeBrowsingSslProxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (!safebrowsing_endpoint.empty() &&
      safebrowsingcrxlist_pattern->MatchesHost(request_url)) {
    replacements.SetHostStr(kBraveSafeBrowsing2Proxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (autofill_pattern->MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveStaticProxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (favicon_pattern->MatchesURL(request_url)) {
    replacements.SetHostStr("favicons.proxy.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (crlSet_pattern1->MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("redirector.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (crlSet_pattern2->MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("redirector.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (crlSet_pattern3->MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("redirector.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (crlSet_pattern4->MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("redirector.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (gvt1_pattern->MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveRedirectorProxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }

  if (googleDl_pattern->MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveRedirectorProxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return;
  }
}

}  // namespace brave

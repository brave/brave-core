/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "base/strings/string_piece_forward.h"
#include "brave/browser/net/brave_geolocation_buildflags.h"
#include "brave/browser/safebrowsing/buildflags.h"
#include "brave/components/constants/network_constants.h"
#include "extensions/common/url_pattern.h"
#include "net/base/net_errors.h"

namespace brave {

const char kSafeBrowsingTestingEndpoint[] = "test.safebrowsing.com";

namespace {

bool g_safebrowsing_api_endpoint_for_testing_ = false;

base::StringPiece GetSafeBrowsingEndpoint() {
  if (g_safebrowsing_api_endpoint_for_testing_)
    return kSafeBrowsingTestingEndpoint;
  return BUILDFLAG(SAFEBROWSING_ENDPOINT);
}

}  // namespace

void SetSafeBrowsingEndpointForTesting(bool testing) {
  g_safebrowsing_api_endpoint_for_testing_ = testing;
}

int OnBeforeURLRequest_StaticRedirectWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  GURL new_url;
  int rc = OnBeforeURLRequest_StaticRedirectWorkForGURL(ctx->request_url,
                                                        &new_url);
  if (!new_url.is_empty()) {
    ctx->new_url_spec = new_url.spec();
  }
  return rc;
}

int OnBeforeURLRequest_StaticRedirectWorkForGURL(
    const GURL& request_url,
    GURL* new_url) {
  GURL::Replacements replacements;
  static URLPattern geo_pattern(URLPattern::SCHEME_HTTPS, kGeoLocationsPattern);
  static URLPattern safeBrowsing_pattern(URLPattern::SCHEME_HTTPS,
                                         kSafeBrowsingPrefix);
  static URLPattern safebrowsingfilecheck_pattern(URLPattern::SCHEME_HTTPS,
                                                  kSafeBrowsingFileCheckPrefix);
  static URLPattern safebrowsingcrxlist_pattern(URLPattern::SCHEME_HTTPS,
                                                kSafeBrowsingCrxListPrefix);

  // To-Do (@jumde) - Update the naming for the variables below
  // https://github.com/brave/brave-browser/issues/10314
  static URLPattern crlSet_pattern1(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kCRLSetPrefix1);
  static URLPattern crlSet_pattern2(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kCRLSetPrefix2);
  static URLPattern crlSet_pattern3(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kCRLSetPrefix3);
  static URLPattern crlSet_pattern4(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kCRLSetPrefix4);
  static URLPattern crxDownload_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kCRXDownloadPrefix);
  static URLPattern autofill_pattern(
      URLPattern::SCHEME_HTTPS, kAutofillPrefix);
  static URLPattern gvt1_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, "*://*.gvt1.com/*");
  static URLPattern googleDl_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      "*://dl.google.com/*");

  static URLPattern widevine_gvt1_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      kWidevineGvt1Prefix);
  static URLPattern widevine_google_dl_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      kWidevineGoogleDlPrefix);

  if (geo_pattern.MatchesURL(request_url)) {
    *new_url = GURL(BUILDFLAG(GOOGLEAPIS_URL));
    return net::OK;
  }

  auto safebrowsing_endpoint = GetSafeBrowsingEndpoint();
  if (!safebrowsing_endpoint.empty() &&
      safeBrowsing_pattern.MatchesHost(request_url)) {
    replacements.SetHostStr(safebrowsing_endpoint);
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (!safebrowsing_endpoint.empty() &&
      safebrowsingfilecheck_pattern.MatchesHost(request_url)) {
    replacements.SetHostStr(kBraveSafeBrowsingSslProxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (!safebrowsing_endpoint.empty() &&
      safebrowsingcrxlist_pattern.MatchesHost(request_url)) {
    replacements.SetHostStr(kBraveSafeBrowsing2Proxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (crxDownload_pattern.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("crxdownload.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (autofill_pattern.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveStaticProxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (crlSet_pattern1.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("redirector.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (crlSet_pattern2.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("redirector.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (crlSet_pattern3.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("redirector.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (crlSet_pattern4.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("redirector.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (gvt1_pattern.MatchesURL(request_url) &&
      !widevine_gvt1_pattern.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveRedirectorProxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (googleDl_pattern.MatchesURL(request_url) &&
      !widevine_google_dl_pattern.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveRedirectorProxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  return net::OK;
}

}  // namespace brave

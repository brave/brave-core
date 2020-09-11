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
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/common/network_constants.h"
#include "brave/common/translate_network_constants.h"
#include "extensions/common/url_pattern.h"
#include "net/base/net_errors.h"

namespace brave {

const char kSafeBrowsingTestingEndpoint[] = "test.safebrowsing.com";

namespace {

bool g_safebrowsing_api_endpoint_for_testing_ = false;

base::StringPiece GetSafeBrowsingEndpoint() {
  if (g_safebrowsing_api_endpoint_for_testing_)
    return kSafeBrowsingTestingEndpoint;
  return SAFEBROWSING_ENDPOINT;
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

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  static URLPattern translate_pattern(URLPattern::SCHEME_HTTPS,
      kTranslateElementJSPattern);
  static URLPattern translate_language_pattern(URLPattern::SCHEME_HTTPS,
      kTranslateLanguagePattern);
#endif
  if (geo_pattern.MatchesURL(request_url)) {
    *new_url = GURL(GOOGLEAPIS_ENDPOINT GOOGLEAPIS_API_KEY);
    return net::OK;
  }

  auto safebrowsing_endpoint = GetSafeBrowsingEndpoint();
  if (!safebrowsing_endpoint.empty() &&
      safeBrowsing_pattern.MatchesHost(request_url)) {
    replacements.SetHostStr(safebrowsing_endpoint);
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (safebrowsingfilecheck_pattern.MatchesHost(request_url)) {
    // TODO(@fmarier): Re-enable download protection once we have
    // truncated the list of metadata that it sends to the server
    // (brave/brave-browser#6267).
    //
    // replacements.SetHostStr(kBraveSafeBrowsingFileCheckProxy);
    // *new_url = request_url.ReplaceComponents(replacements);
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
    replacements.SetHostStr("crlsets.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (crlSet_pattern2.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("crlsets.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (crlSet_pattern3.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("crlsets.brave.com");
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (crlSet_pattern4.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("crlsets.brave.com");
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

#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  if (translate_pattern.MatchesURL(request_url)) {
    replacements.SetQueryStr(request_url.query_piece());
    replacements.SetPathStr(request_url.path_piece());
    *new_url =
      GURL(kBraveTranslateEndpoint).ReplaceComponents(replacements);
    return net::OK;
  }

  if (translate_language_pattern.MatchesURL(request_url)) {
    *new_url = GURL(kBraveTranslateLanguageEndpoint);
    return net::OK;
  }
#endif

  return net::OK;
}


}  // namespace brave

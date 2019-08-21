/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

#include <memory>
#include <vector>

#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/common/network_constants.h"
#include "brave/common/translate_network_constants.h"
#include "extensions/common/url_pattern.h"

namespace brave {

int OnBeforeURLRequest_StaticRedirectWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  GURL::Replacements replacements;
  static URLPattern geo_pattern(URLPattern::SCHEME_HTTPS, kGeoLocationsPattern);
  static URLPattern safeBrowsing_pattern(URLPattern::SCHEME_HTTPS,
                                         kSafeBrowsingPrefix);
  static URLPattern safebrowsingfilecheck_pattern(URLPattern::SCHEME_HTTPS,
                                         kSafeBrowsingFileCheckPrefix);
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
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  static URLPattern translate_pattern(URLPattern::SCHEME_HTTPS,
      kTranslateElementJSPattern);
  static URLPattern translate_language_pattern(URLPattern::SCHEME_HTTPS,
      kTranslateLanguagePattern);
#endif
  if (geo_pattern.MatchesURL(ctx->request_url)) {
    ctx->new_url_spec = GURL(GOOGLEAPIS_ENDPOINT GOOGLEAPIS_API_KEY).spec();
    return net::OK;
  }

  if (safeBrowsing_pattern.MatchesHost(ctx->request_url)) {
    replacements.SetHostStr(SAFEBROWSING_ENDPOINT);
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }

  if (safebrowsingfilecheck_pattern.MatchesHost(ctx->request_url)) {
    replacements.SetHostStr(kBraveSafeBrowsingFileCheckProxy);
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }

  if (crxDownload_pattern.MatchesURL(ctx->request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("crxdownload.brave.com");
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }

  if (crlSet_pattern1.MatchesURL(ctx->request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("crlsets.brave.com");
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }

  if (crlSet_pattern2.MatchesURL(ctx->request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("crlsets.brave.com");
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }

  if (crlSet_pattern3.MatchesURL(ctx->request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("crlsets.brave.com");
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }

  if (crlSet_pattern4.MatchesURL(ctx->request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr("crlsets.brave.com");
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  if (translate_pattern.MatchesURL(ctx->request_url)) {
    replacements.SetQueryStr(ctx->request_url.query_piece());
    replacements.SetPathStr(ctx->request_url.path_piece());
    ctx->new_url_spec =
      GURL(kBraveTranslateEndpoint).ReplaceComponents(replacements).spec();
    return net::OK;
  }

  if (translate_language_pattern.MatchesURL(ctx->request_url)) {
    ctx->new_url_spec = GURL(kBraveTranslateLanguageEndpoint).spec();
    return net::OK;
  }
#endif

#if !defined(NDEBUG)
  GURL gurl = ctx->request_url;
  static std::vector<URLPattern> allowed_patterns({
      // Brave updates
      URLPattern(URLPattern::SCHEME_HTTPS, "https://go-updater.brave.com/*"),
      // Brave promo referrals, production and staging (laptop-updates
      // proxies to promo-services)
      // TODO(@emerick): In the future, we may want to specify the value of the
      // BRAVE_REFERRALS_SERVER environment variable rather than
      // hardcoding the server name here
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://laptop-updates.brave.com/*"),
      // CRX file download
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://brave-core-ext.s3.brave.com/release/*"),
      // Safe Browsing and other files
      URLPattern(URLPattern::SCHEME_HTTPS, "https://static.brave.com/*"),
      // We do allow redirects to the Google update server for extensions we
      // don't support
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://update.googleapis.com/service/update2/json"),

      // Rewards URLs
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://ledger.mercury.basicattentiontoken.org/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://balance.mercury.basicattentiontoken.org/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://publishers.basicattentiontoken.org/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://publishers-distro.basicattentiontoken.org/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://ledger-staging.mercury.basicattentiontoken.org/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://balance-staging.mercury.basicattentiontoken.org/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://publishers-staging.basicattentiontoken.org/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://publishers-staging-distro.basicattentiontoken.org/*"),

      // Safe browsing
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://safebrowsing.brave.com/v4/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
          "https://ssl.gstatic.com/safebrowsing/*"),

      URLPattern(URLPattern::SCHEME_HTTPS, "https://crlsets.brave.com/*"),
      URLPattern(URLPattern::SCHEME_HTTPS, "https://crxdownload.brave.com/*"),

      // Brave's translation relay server
      URLPattern(URLPattern::SCHEME_HTTPS, kBraveTranslateServerPrefix),
  });

  // Check to make sure the URL being requested matches at least one of the
  // allowed patterns
  bool is_url_allowed =
      std::any_of(allowed_patterns.begin(), allowed_patterns.end(),
                  [&gurl](URLPattern pattern) {
                    if (pattern.MatchesURL(gurl)) {
                      return true;
                    }
                    return false;
                  });
  if (!is_url_allowed) {
    LOG(ERROR) << "URL not allowed from system network delegate: " << gurl;
  }
  // TODO(@bbondy): Before we can turn this into DCHECK we have to find a way to
  // allow these, I think they are for Chrome Cast
  // http://192.168.0.13:8008/ssdp/device-desc.xml
  // http://192.168.0.27:60000/upnp/dev/e16bf493-ed87-5798-ffff-ffffeb4f1c34/desc
  // And also I don't know where they're from, but there's always 3 requests
  // similar to this: http://vijscbncpv/
#endif

  return net::OK;
}

}  // namespace brave

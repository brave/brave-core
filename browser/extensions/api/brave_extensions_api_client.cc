/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_extensions_api_client.h"

#include "base/strings/string_piece.h"
#include "brave/common/url_constants.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/url_pattern.h"
#include "url/origin.h"

namespace extensions {

bool BraveExtensionsAPIClient::ShouldHideBrowserNetworkRequest(
    content::BrowserContext* context,
    const WebRequestInfo& request) const {
  const url::Origin origin = url::Origin::Create(request.url);
  const base::StringPiece path = request.url.path_piece();
  if (((origin.DomainIs("sandbox.uphold.com") ||
        origin.DomainIs("uphold.com")) &&
       base::StartsWith(path, "/authorize/",
                        base::CompareCase::INSENSITIVE_ASCII)) ||
      (origin.DomainIs("api.uphold.com") &&
       base::StartsWith(path, "/oauth2/token",
                        base::CompareCase::INSENSITIVE_ASCII))) {
    return true;  // protected URL
  }

  // Binance
  URLPattern pattern1(URLPattern::SCHEME_HTTPS,
      "https://accounts.binance.com/*/oauth/authorize*");
  URLPattern pattern2(URLPattern::SCHEME_HTTPS,
      "https://accounts.binance.com/oauth/token*");
  if (pattern1.MatchesURL(request.url) || pattern2.MatchesURL(request.url)) {
    return true;
  }

  if (request.url.SchemeIs(kBinanceScheme)) {
    return true;
  }

  // Gemini
  URLPattern auth_pattern(URLPattern::SCHEME_HTTPS,
      "https://exchange.gemini.com/auth*");
  URLPattern token_pattern(URLPattern::SCHEME_HTTPS,
      "https://exchange.gemini.com/auth/token*");
  if (auth_pattern.MatchesURL(request.url) ||
      token_pattern.MatchesURL(request.url)) {
    return true;
  }

  if (request.url.SchemeIs(kGeminiScheme)) {
    return true;
  }

  // FTX
  URLPattern ftx_us_auth_pattern(URLPattern::SCHEME_HTTPS,
      "https://ftx.us/oauth*");
  URLPattern ftx_us_token_pattern(URLPattern::SCHEME_HTTPS,
      "https://ftx.us/api/oauth/token*");
  URLPattern ftx_com_auth_pattern(URLPattern::SCHEME_HTTPS,
      "https://ftx.com/oauth*");
  URLPattern ftx_com_token_pattern(URLPattern::SCHEME_HTTPS,
      "https://ftx.com/api/oauth/token*");

  if (ftx_us_auth_pattern.MatchesURL(request.url) ||
      ftx_us_token_pattern.MatchesURL(request.url) ||
      ftx_com_auth_pattern.MatchesURL(request.url) ||
      ftx_com_token_pattern.MatchesURL(request.url)) {
    return true;
  }

  if (request.url.SchemeIs(kFTXScheme)) {
    return true;
  }

  return ChromeExtensionsAPIClient::ShouldHideBrowserNetworkRequest(context,
                                                                    request);
}

}  // namespace extensions

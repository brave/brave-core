/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/net/ios_chrome_network_delegate.h"

#define IOSChromeNetworkDelegate IOSChromeNetworkDelegate_ChromiumImpl
#include "src/ios/chrome/browser/net/ios_chrome_network_delegate.cc"
#undef IOSChromeNetworkDelegate

#include "brave/common/network_constants.h"
#include "extensions/common/url_pattern.h"

namespace {

void AddBraveServicesKeyHeader(net::URLRequest* request) {
  static URLPattern brave_proxy_pattern(URLPattern::SCHEME_HTTPS,
                                        kBraveProxyPattern);
  static URLPattern bravesoftware_proxy_pattern(URLPattern::SCHEME_HTTPS,
                                                kBraveSoftwareProxyPattern);
  if (brave_proxy_pattern.MatchesURL(request->url()) ||
      bravesoftware_proxy_pattern.MatchesURL(request->url())) {
    request->SetExtraRequestHeaderByName(
        kBraveServicesKeyHeader, BRAVE_SERVICES_KEY, true /* overrwrite */);
  }
}

}  // namespace

IOSChromeNetworkDelegate::~IOSChromeNetworkDelegate() = default;

int IOSChromeNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  const auto result = IOSChromeNetworkDelegate_ChromiumImpl::OnBeforeURLRequest(
      request, std::move(callback), new_url);
  ::AddBraveServicesKeyHeader(request);
  return result;
}

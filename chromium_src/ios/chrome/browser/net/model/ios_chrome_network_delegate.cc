/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/net/model/ios_chrome_network_delegate.h"

#define IOSChromeNetworkDelegate IOSChromeNetworkDelegate_ChromiumImpl
#include "src/ios/chrome/browser/net/model/ios_chrome_network_delegate.cc"
#undef IOSChromeNetworkDelegate

#include "brave/components/constants/brave_services_key.h"
#include "brave/components/constants/brave_services_key_helper.h"
#include "brave/components/constants/network_constants.h"

namespace {

void AddBraveServicesKeyHeader(net::URLRequest* request) {
  if (brave::ShouldAddBraveServicesKeyHeader(request->url())) {
    request->SetExtraRequestHeaderByName(kBraveServicesKeyHeader,
                                         BUILDFLAG(BRAVE_SERVICES_KEY),
                                         true /* overrwrite */);
  }
}

}  // namespace

IOSChromeNetworkDelegate::~IOSChromeNetworkDelegate() = default;

int IOSChromeNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  ::AddBraveServicesKeyHeader(request);
  return net::OK;
}

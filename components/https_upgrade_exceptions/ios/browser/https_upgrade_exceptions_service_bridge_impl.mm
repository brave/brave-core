// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/https_upgrade_exceptions/ios/browser/https_upgrade_exceptions_service_bridge_impl.h"

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "brave/components/https_upgrade_exceptions/core/browser/https_upgrade_exceptions_service.h"
#include "net/base/apple/url_conversions.h"
#include "net/base/features.h"
#include "url/gurl.h"

@implementation HTTPSUpgradeExceptionsServiceBridgeImpl

- (instancetype)initWithHTTPSUpgradeExceptionsService:
    (raw_ptr<https_upgrade_exceptions::HttpsUpgradeExceptionsService>)
        httpsUpgradeExceptionsService {
  if ((self = [super init])) {
    _httpsUpgradeExceptionsService = httpsUpgradeExceptionsService;
  }
  return self;
}

- (BOOL)canUpgradeToHTTPSForURL:(NSURL*)url {
  if (!base::FeatureList::IsEnabled(net::features::kBraveHttpsByDefault)) {
    return false;
  }

  GURL gurl = net::GURLWithNSURL(url);
  // Check url validity
  if (!gurl.SchemeIs("http") || !gurl.is_valid()) {
    return false;
  }

  return self.httpsUpgradeExceptionsService->CanUpgradeToHTTPS(gurl);
}

@end

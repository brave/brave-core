// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/https_upgrade_exceptions/https_upgrade_exceptions_service+private.h"

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "net/base/apple/url_conversions.h"
#include "net/base/features.h"
#include "url/gurl.h"
#include "url/origin.h"

@interface HTTPSUpgradeExceptionsService () {
  /// set of domain exceptions
  std::set<std::string> exceptional_domains_;
}

@property(nonatomic) bool isHttpsByDefaultFeatureEnabled;
@end

@implementation HTTPSUpgradeExceptionsService
- (instancetype)init {
  if ((self = [super init])) {
  }
  return self;
}

- (bool)isHttpsByDefaultFeatureEnabled {
  return base::FeatureList::IsEnabled(net::features::kBraveHttpsByDefault);
}

- (NSURL*)upgradeToHTTPSForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  if (![self isHttpsByDefaultFeatureEnabled]) {
    return nil;
  }

  // Check url validity
  if (!gurl.SchemeIs("http") || !gurl.is_valid()) {
    return nil;
  }

  // Ensure we didn't add an exception for this domain
  if (base::Contains(exceptional_domains_, gurl.host())) {
    return nil;
  }

  // Finally ask the service if we should upgrade
  BraveApplicationContextImpl* braveContext =
      static_cast<BraveApplicationContextImpl*>(GetApplicationContext());
  if (!braveContext->https_upgrade_exceptions_service()->CanUpgradeToHTTPS(
          gurl)) {
    return nil;
  }

  GURL::Replacements replacements;
  replacements.SetSchemeStr("https");
  GURL final_url = gurl.ReplaceComponents(replacements);

  if (final_url.is_valid()) {
    return net::NSURLWithGURL(final_url);
  } else {
    return nil;
  }
}

- (void)addExceptionForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);

  if (gurl.is_empty()) {
    return;
  }

  exceptional_domains_.insert(gurl.host());
}
@end

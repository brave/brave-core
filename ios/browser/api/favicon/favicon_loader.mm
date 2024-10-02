/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/favicon/favicon_loader.h"

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#import "brave/ios/browser/favicon/brave_ios_favicon_loader.h"
#import "brave/ios/browser/favicon/brave_ios_favicon_loader_factory.h"
#include "components/favicon_base/favicon_types.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/chrome/common/ui/favicon/favicon_attributes.h"
#import "ios/chrome/common/ui/favicon/favicon_constants.h"
#import "net/base/apple/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Constants
FaviconLoaderSize const FaviconLoaderSizeDesiredSmallest =
    static_cast<NSInteger>(kMinFaviconSizePt);
FaviconLoaderSize const FaviconLoaderSizeDesiredSmall =
    static_cast<NSInteger>(kDesiredSmallFaviconSizePt);
FaviconLoaderSize const FaviconLoaderSizeDesiredMedium =
    static_cast<NSInteger>(kDesiredMediumFaviconSizePt);
FaviconLoaderSize const FaviconLoaderSizeDesiredLarge =
    static_cast<NSInteger>(64);
FaviconLoaderSize const FaviconLoaderSizeDesiredLarger =
    static_cast<NSInteger>(180);
FaviconLoaderSize const FaviconLoaderSizeDesiredLargest =
    static_cast<NSInteger>(192);

// MARK: - Implementation

@interface FaviconLoader () {
  base::raw_ptr<brave_favicon::BraveFaviconLoader> favicon_loader_;
}
@end

@implementation FaviconLoader
- (instancetype)initWithBrowserState:(ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    favicon_loader_ =
        brave_favicon::BraveIOSFaviconLoaderFactory::GetForBrowserState(
            browserState);
    DCHECK(favicon_loader_);
  }
  return self;
}

+ (instancetype)getForPrivateMode:(bool)privateMode {
  std::vector<ProfileIOS*> profiles =
      GetApplicationContext()->GetProfileManager()->GetLoadedProfiles();
  ProfileIOS* last_used_profile = profiles.at(0);

  if (privateMode) {
    last_used_profile = last_used_profile->GetOffTheRecordProfile();
    CHECK(last_used_profile);
  }

  return [[FaviconLoader alloc] initWithBrowserState:last_used_profile];
}

- (void)faviconForPageURLOrHost:(NSURL*)url
                   sizeInPoints:(FaviconLoaderSize)sizeInPoints
                minSizeInPoints:(FaviconLoaderSize)minSizeInPoints
                     completion:
                         (void (^)(FaviconLoader* loader,
                                   FaviconAttributes* attributes))completion {
  favicon_loader_->FaviconForPageUrlOrHost(net::GURLWithNSURL(url),
                                           sizeInPoints, minSizeInPoints,
                                           ^(FaviconAttributes* attributes) {
                                             completion(self, attributes);
                                           });
}
@end

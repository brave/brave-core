/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/favicon/favicon_loader.h"
#include "base/strings/sys_string_conversions.h"
#import "brave/ios/browser/favicon/brave_ios_favicon_loader.h"
#import "brave/ios/browser/favicon/brave_ios_favicon_loader_factory.h"
#include "components/favicon_base/favicon_types.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/common/ui/favicon/favicon_attributes.h"
#import "ios/chrome/common/ui/favicon/favicon_constants.h"
#import "net/base/mac/url_conversions.h"

#include <memory>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Constants
BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredSmallest =
    static_cast<NSInteger>(kMinFaviconSizePt);
BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredSmall =
    static_cast<NSInteger>(kDesiredSmallFaviconSizePt);
BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredMedium =
    static_cast<NSInteger>(kDesiredMediumFaviconSizePt);
BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredLarge =
    static_cast<NSInteger>(64);
BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredLarger =
    static_cast<NSInteger>(180);
BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredLargest =
    static_cast<NSInteger>(192);

// MARK: - Implementation

@interface BraveFaviconLoader () {
  brave_favicon::BraveFaviconLoader* brave_favicon_loader_;
}
@end

@implementation BraveFaviconLoader
- (instancetype)initWithBrowserState:(ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    brave_favicon_loader_ =
        brave_favicon::BraveIOSFaviconLoaderFactory::GetForBrowserState(
            browserState);
    DCHECK(brave_favicon_loader_);
  }
  return self;
}

+ (instancetype)getForPrivateMode:(bool)privateMode {
  ios::ChromeBrowserStateManager* browser_state_manager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  CHECK(browser_state_manager);

  ChromeBrowserState* browser_state =
      browser_state_manager->GetLastUsedBrowserState();
  CHECK(browser_state);

  if (privateMode) {
    browser_state = browser_state->GetOffTheRecordChromeBrowserState();
    CHECK(browser_state);
  }

  return [[BraveFaviconLoader alloc] initWithBrowserState:browser_state];
}

- (void)faviconForPageURLOrHost:(NSURL*)url
                   sizeInPoints:(BraveFaviconLoaderSize)sizeInPoints
                minSizeInPoints:(BraveFaviconLoaderSize)minSizeInPoints
                     completion:
                         (void (^)(FaviconAttributes* attributes))completion {
  brave_favicon_loader_->FaviconForPageUrlOrHost(
      net::GURLWithNSURL(url), sizeInPoints, minSizeInPoints,
      ^(FaviconAttributes* attributes) {
        completion(attributes);
      });
}
@end

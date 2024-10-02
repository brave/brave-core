/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/opentabs/brave_tabgenerator_api.h"

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#import "brave/ios/browser/api/web/web_state/web_state+private.h"
#include "ios/chrome/browser/shared/model/browser/browser.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web/web_state/web_state_impl.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - BraveSyncTab

@interface BraveSyncTab ()
@property(nonatomic, strong) WebState* web_state;
@end

@implementation BraveSyncTab

- (instancetype)initWithBrowser:(Browser*)browser
                 isOffTheRecord:(bool)isOffTheRecord {
  if ((self = [super init])) {
    _web_state = [[WebState alloc] initWithBrowser:browser
                                    isOffTheRecord:isOffTheRecord];
  }
  return self;
}

- (WebState*)webState {
  return _web_state;
}

- (void)setTitle:(NSString*)title {
  [self.web_state setTitle:title];
}

- (void)setURL:(NSURL*)url {
  [self.web_state setURL:url];
}
@end

#pragma mark - BraveTabGeneratorAPI

@interface BraveTabGeneratorAPI () {
  raw_ptr<Browser> browser_;
}
@end

@implementation BraveTabGeneratorAPI

- (instancetype)initWithBrowser:(Browser*)browser {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    DCHECK(browser);
    browser_ = browser;
  }
  return self;
}

- (BraveSyncTab*)createBraveSyncTab:(bool)isOffTheRecord {
  return [[BraveSyncTab alloc] initWithBrowser:browser_
                                isOffTheRecord:isOffTheRecord];
}

- (void)dealloc {
  browser_ = nil;
}

@end

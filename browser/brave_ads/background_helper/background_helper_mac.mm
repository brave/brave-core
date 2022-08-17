/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/background_helper/background_helper_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_logging.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"

@interface BackgroundHelperDelegateMac : NSObject {
 @private
  brave_ads::BackgroundHelper* helper_;  // NOT OWNED
}

- (void)appDidBecomeActive:(NSNotification*)notification;
- (void)appDidResignActive:(NSNotification*)notification;

@end

@implementation BackgroundHelperDelegateMac

- (id)initWithHelper:(brave_ads::BackgroundHelper*)helper {
  if ((self = [super init])) {
    helper_ = helper;

    NSNotificationCenter* notificationCenter =
        [NSNotificationCenter defaultCenter];
    [notificationCenter addObserver:self
                           selector:@selector(appDidBecomeActive:)
                               name:NSApplicationDidBecomeActiveNotification
                             object:nil];
    [notificationCenter addObserver:self
                           selector:@selector(appDidResignActive:)
                               name:NSApplicationDidResignActiveNotification
                             object:nil];
  }
  return self;
}

- (void)appDidBecomeActive:(NSNotification*)notification {
  helper_->TriggerOnForeground();
}

- (void)appDidResignActive:(NSNotification*)notification {
  helper_->TriggerOnBackground();
}

@end

namespace brave_ads {

class BackgroundHelperMac::BackgroundHelperDelegate {
 public:
  BackgroundHelperDelegate(BackgroundHelper* background_helper) {
    delegate_.reset(
        [[BackgroundHelperDelegateMac alloc] initWithHelper:background_helper]);
  }

  ~BackgroundHelperDelegate() = default;

 private:
  base::scoped_nsobject<BackgroundHelperDelegateMac> delegate_;
};

BackgroundHelperMac::BackgroundHelperMac() {
  delegate_ = std::make_unique<BackgroundHelperDelegate>(this);
}

BackgroundHelperMac::~BackgroundHelperMac() = default;

bool BackgroundHelperMac::IsForeground() const {
  return [[NSApplication sharedApplication] isActive];
}

}  // namespace brave_ads

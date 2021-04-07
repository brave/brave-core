/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/notifications/notification_platform_bridge_mac.h"

#import <Foundation/Foundation.h>

#include "base/mac/mac_util.h"

@interface NotificationTimeoutMac : NSObject

- (void)startTimer:(NSUserNotification *)notification;

@end

@implementation NotificationTimeoutMac {
  NSMutableArray<NSTimer *> * timers_;
}

- (id)init {
  if (self = [super init]) {
    timers_ = [[NSMutableArray alloc] init];
  }

  return self;
}

- (void)dealloc {
  for (NSTimer * timer in timers_) {
    [timer invalidate];
    timer = nil;
  }

  [timers_ removeAllObjects];

  [timers_ release];
  timers_ = nil;

  [super dealloc];
}

- (void)startTimer:(NSUserNotification *)notification {
  NSTimer * timer =
      [NSTimer scheduledTimerWithTimeInterval:5.0
                                       target:self
                                     selector:@selector(timerFired:)
                                     userInfo:[notification retain]
                                      repeats:NO];
    [timers_ addObject:timer];
}

- (void)timerFired:(NSTimer *)timer {
  NSUserNotification* notification = timer.userInfo;
  if (!notification) {
    return;
  }

  auto* notification_center =
      [NSUserNotificationCenter defaultUserNotificationCenter];
  [notification_center removeDeliveredNotification:notification];

  [notification release];
  notification = nil;

  [timers_ removeObject:timer];
}

@end

static NotificationTimeoutMac*
    g_notification_platform_bridge_notification_timeout =
        [[[NotificationTimeoutMac alloc] init] retain];

#define BRAVE_DISPLAY_ \
  [g_notification_platform_bridge_notification_timeout startTimer:toast];

#include "../../../../../chrome/browser/notifications/notification_platform_bridge_mac.mm"  // NOLINT
#undef BRAVE_DISPLAY_

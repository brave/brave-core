/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

@interface BraveCoreMain : NSObject

- (instancetype)init;

- (void)scheduleLowPriorityStartupTasks;

- (void)setUserAgent:(NSString *)userAgent;

@end

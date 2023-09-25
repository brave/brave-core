/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/browser/memory/memory_debugger.h"
#include <stdint.h>
#include <memory>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MemoryDebugger {
}

- (instancetype)init {
  self = [super initWithFrame:CGRectZero];
  return self;
}


- (void)invalidateTimers {}

@end

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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

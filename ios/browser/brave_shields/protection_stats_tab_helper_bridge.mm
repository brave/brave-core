// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/protection_stats_tab_helper_bridge.h"

@implementation ProtectionStatsResource

- (instancetype)initWithURL:(NSString*)url type:(NSString*)type {
  if ((self = [super init])) {
    _resourceURL = [url copy];
    _resourceType = [type copy];
  }
  return self;
}

@end
